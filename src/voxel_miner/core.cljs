(ns voxel-miner.core
  (:require [dommy.core :as dom]

            [thi.ng.math.core :as m :refer [PI HALF_PI TWO_PI]]
            [thi.ng.color.core :as col]
            [thi.ng.typedarrays.core :as arrays]
            [thi.ng.geom.gl.core :as gl]
            [thi.ng.geom.gl.webgl.constants :as glc]
            [thi.ng.geom.gl.webgl.animator :as anim]
            [thi.ng.geom.gl.buffers :as buf]
            [thi.ng.geom.gl.shaders :as sh]
            [thi.ng.geom.gl.shaders.phong :as phong]
            [thi.ng.geom.gl.utils :as glu]
            [thi.ng.geom.gl.glmesh :as glm]
            [thi.ng.geom.gl.camera :as cam]
            [thi.ng.geom.core :as g]
            [thi.ng.geom.vector :as v :refer [vec2 vec3]]
            [thi.ng.geom.matrix :as mat :refer [M44]]
            [thi.ng.geom.aabb :as a]
            [thi.ng.geom.attribs :as attr]
            [thi.ng.glsl.core :as glsl :include-macros true]

            [thi.ng.geom.circle :as c]
            [thi.ng.geom.ptf :as ptf]
            [thi.ng.glsl.vertex :as vertex]
            [thi.ng.glsl.lighting :as light]
            [thi.ng.glsl.fog :as fog]
            [thi.ng.color.gradients :as grad]

            [thi.ng.math.noise :as n]
            [voxel-miner.chunks :as chunks]
            [voxel-miner.matrix :as matrix]
            [voxel-miner.texture :as t]))

(enable-console-print!)

(defonce canvas (dom/sel1 :#main))
(defonce gl-ctx (gl/gl-context "main"))
(defonce view-rect (gl/get-viewport-rect gl-ctx))

(defonce m-matrix (atom matrix/i4))

(defonce drag (atom false))
(defonce time-old (atom 0))

(defonce theta (atom 0))
(defonce phi (atom 0))
(defonce dx (atom 0))
(defonce dy (atom 0))
(defonce old-x (atom 0))
(defonce old-y (atom 0))

(def canvas-width (aget gl-ctx "canvas" "width"))
(def canvas-height (aget gl-ctx "canvas" "height"))
(def aspect-ratio (/ canvas-width canvas-height))

(def shader-spec
  {:vs (->> "void main() {
                vUV = uv;
                vPos = (view * model * vec4(position, 1.0)).xyz;
                vNormal = surfaceNormal(normal, normalMat);
                vLightDir = (view * vec4(lightPos, 1.0)).xyz - vPos;
                gl_Position = proj * vec4(vPos, 1.0);
              }"
            (glsl/glsl-spec [vertex/surface-normal])
            (glsl/assemble))
   :fs (->> "void main() {
                vec3 n = normalize(vNormal);
                vec3 v = normalize(-vPos);
                vec3 l = normalize(vLightDir);
                float NdotL = max(0.0, dot(n, l));
                vec3 specular = Ks * phong(l, v, n);
                vec3 att = lightCol / pow(length(vLightDir), lightAtt);
                vec3 diff = texture2D(tex, vUV).xyz;
                vec3 col = att * NdotL * ((1.0 - s) * diff + s * specular) + Ka * diff;
                float fog = fogLinear(length(vPos), 1.0, 200.0);
                col = mix(col, Kf, fog);
                gl_FragColor = vec4(col, 1.0);
              }"
            (glsl/glsl-spec [fog/fog-linear light/phong])
            (glsl/assemble))
   :uniforms {:model     :mat4
              :view      :mat4
              :proj      :mat4
              :normalMat :mat4
              :tex       :sampler2D
              :Ks        [:vec3 [0.1 0.1 0.1]]
              :Ka        [:vec3 [1 1 1]]
              :Kf        [:vec3 [0.7 0.8 1]]
              :m         [:float 0.1]
              :s         [:float 0.1]
              :lightCol  [:vec3 [100 100 100]]
              :lightPos  [:vec3 [0 0 25]]
              :lightAtt  [:float 3.0]
              :time      :float}
   :attribs  {:position :vec3
              :normal   :vec3
              :uv       :vec2}
   :varying  {:vUV      :vec2
              :vPos     :vec3
              :vNormal  :vec3
              :vLightDir :vec3}
   :state    {:depth-test true}})

(def p-matrix (gl/perspective 90 aspect-ratio 1 100))
(def v-matrix (flatten (matrix/translate-z matrix/i4 -10)))

(def n-scale 0.1)
(def iso-val 0.33)

(defn make-world [w h d]
  (for [x (range (- w) w)
        y (range (- h) 1)
        z (range (- d) d)
        :when (or
               (= y (- h))
               (> iso-val (m/abs* (n/noise3 (* x n-scale) (* y n-scale) (* z n-scale)))))]
    (cond
      (= y 0) {:type :grass :position [x y z]}
      (> y (/ (- h) 2)) {:type :dirt :position [x y z]}
      :else {:type :stone :position [x y z]})))

(defn chunk-mesh [chunk-spec]
  (-> chunk-spec
      (merge {:uniforms {:proj p-matrix
                         :view v-matrix}})
      (assoc :shader (sh/make-shader-from-spec gl-ctx shader-spec))
      (gl/make-buffers-in-spec gl-ctx glc/static-draw)))

(def world (time (doall (make-world 30 10 30))))
(def chunk-specs (chunks/chunkify world))
(def chunks (map chunk-mesh chunk-specs))

(defn mouse-down [e]
  (reset! drag true)
  ;; (reset! old-x (.-pageX e))
  ;; (reset! old-y (.-pageY e))
  (.requestPointerLock canvas)
  (.preventDefault e))

(defn mouse-up [e]
  ;; (reset! drag false)
  true)

(defn mouse-move [e]
  (when @drag
    (let [edx (.-movementX e)
          edy (.-movementY e)]
      (reset! dx (/ edx canvas-width))
      (reset! dy (/ edy canvas-height))
      (swap! theta #(+ % @dx))
      (swap! phi #(+ % @dy))
      (reset! old-x (.-movementX e))
      (reset! old-y (.-movementY e)))))

(defonce add-listeners
  (do
    (dom/listen! canvas :mousedown mouse-down)
    (dom/listen! canvas :mouseup mouse-up)
    (dom/listen! canvas :mouseout mouse-up)
    (dom/listen! canvas :mousemove mouse-move)))

(def tex-ready (volatile! false))
(def tex (buf/load-texture
          gl-ctx
          {:callback (fn [tex img] (vreset! tex-ready true))
           :src t/texture-file
           :flip false}))

(def amortization 0)
(defn draw-frame! [t]
  (let [dt (- t @time-old)]
    (when-not @drag
      (swap! dx #(* % amortization))
      (swap! dy #(* % amortization))
      (swap! theta #(+ % @dx))
      (swap! phi #(+ % @dy)))
    (reset! m-matrix (-> matrix/i4
                         (matrix/rotate-x @phi)
                         (matrix/rotate-y @theta)))
    (reset! time-old t))
  (when @tex-ready
    (gl/bind tex 0)
    (doto gl-ctx
      (gl/set-viewport view-rect)
      (gl/clear-color-and-depth-buffer 0.8 0.9 1 1 1)
      (gl/enable glc/depth-test)
      (gl/enable glc/cull-face))
    (doseq [model chunks]
      (gl/draw-with-shader gl-ctx
                           (-> model
                               (assoc-in [:uniforms :model] (flatten @m-matrix))))))
  true)

(defn ^:export demo
  []
  (let [gl        (gl/gl-context "main")
        view-rect (gl/get-viewport-rect gl)
        shader (sh/make-shader-from-spec gl shader-spec)
        models    (map (fn [model]
                         (-> model
                             (cam/apply (cam/perspective-camera
                                         {:eye (vec3 0 0 20) :fov 90 :aspect view-rect}))
                             (assoc :shader shader)
                             (gl/make-buffers-in-spec gl glc/static-draw)))
                       chunk-specs)
        tex-ready (volatile! false)
        tex       (buf/load-texture
                   gl {:callback (fn [tex img] (vreset! tex-ready true))
                       :src      "texture.png"
                       :flip     false})]
    (anim/animate
     (fn [t frame]
       (let [dt (- t @time-old)]
         (when-not @drag
           (swap! dx #(* % amortization))
           (swap! dy #(* % amortization))
           (swap! theta #(+ % @dx))
           (swap! phi #(+ % @dy)))
         (reset! m-matrix (-> matrix/i4
                              (matrix/rotate-x @phi)
                              (matrix/rotate-y @theta)))
         (reset! time-old t))
       (when @tex-ready
         (gl/bind tex 0)
         (doto gl
           (gl/set-viewport view-rect)
           (gl/clear-color-and-depth-buffer 0.7 0.8 1 1 1)
           (gl/enable glc/depth-test)
           (gl/enable glc/cull-face))
         (doseq [model models]
           (gl/draw-with-shader
            gl
            (-> model
                (update :uniforms assoc
                        :time t
                        :m (+ 0.21 (* 0.2 (Math/sin (* t 0.5))))
                        :model (-> M44 (g/rotate-x @phi) (g/rotate-y @theta)))
                (gl/inject-normal-matrix :model :view :normalMat)))))
       true))))

(demo)
