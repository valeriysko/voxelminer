(ns voxel-miner.chunks
  (:require [goog.object :as o]
            [thi.ng.color.core :as col]
            [thi.ng.geom.aabb :as a]
            [thi.ng.geom.attribs :as attr]
            [thi.ng.geom.core :as g]
            [thi.ng.geom.gl.core :as gl]
            [thi.ng.geom.gl.glmesh :as glm]
            [thi.ng.math.core :as m :refer [PI HALF_PI TWO_PI]]
            [thi.ng.math.noise :as n]
            [thi.ng.typedarrays.core :as arrays]
            [voxel-miner.texture :as t]))

(def n-scale 0.1)
(def iso-val 0.2)

(defn- vectorize [chunk]
  (let [blocks (new js/Module.VectorBlock)]
    (doseq [block chunk]
      (.push_back blocks block))
    blocks))

(defn make-world [w h d]
  (for [x (range (- w) w)
        y (range (- h) 1)
        z (range (- d) d)
        :when (> iso-val (m/abs* (n/noise3 (* x n-scale)
                                           (* y n-scale)
                                           (* z n-scale))))]
    (cond
      (= y 0) #js {"type" "grass" "position" #js [x y z]}
      (>= y (/ (- h) 2)) #js {"type" "dirt" "position" #js [x y z]}
      :else #js {"type" "stone" "position" #js [x y z]})))

(def chunk-size 16)
(defn- get-chunk [block]
  (let [pos (o/get block "position")]
    [(quot (first pos) chunk-size)
     (quot (last pos) chunk-size)]))

(defn- chunk-spec [blocks]
  (def face-vector [0 1 2 0 2 3])
  (let [chunky (js/Module.chunkify (vectorize blocks))
        render-count (aget chunky 0)
        posd (aget chunky 1)
        normd (aget chunky 2)
        uvd (aget chunky 3)
        indices (aget chunky 4)]
    {:attribs {:position {:data posd
                          :size 3}
               :normal {:data normd
                        :size 3}
               :uv {:data uvd
                    :size 2}}
     :indices {:data indices}
     :num-items (* render-count 2 3)
     :num-vertices (* render-count 2 2)
     :mode 4}))

(defn chunkify [world]
  (->> world
       (group-by get-chunk)
       (vals)
       (map chunk-spec)))
