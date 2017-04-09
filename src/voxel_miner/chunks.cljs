(ns voxel-miner.chunks
  (:require [thi.ng.color.core :as col]
            [thi.ng.geom.aabb :as a]
            [thi.ng.geom.attribs :as attr]
            [thi.ng.geom.core :as g]
            [thi.ng.geom.gl.core :as gl]
            [thi.ng.geom.gl.glmesh :as glm]
            [thi.ng.typedarrays.core :as arrays]
            [voxel-miner.texture :as t]))

(defn- get-chunk [block]
  (def chunk-size 16)
  (map #(quot % chunk-size) (:position block)))

(defn- block-faces [coord]
  (def cube (a/aabb 1))
  (-> cube
      (g/translate coord)
      (g/faces)))

(defn- block-normal [block]
  (map #(repeat 4 %) [[1 0 0]
                      [-1 0 0]
                      [0 1 0]
                      [0 -1 0]
                      [0 0 1]
                      [0 0 -1]]))

(defn- chunk-spec [blocks]
  (def face-vector [0 1 2 0 2 3])
  (let [position-data (->> blocks
                           (map :position)
                           (mapcat block-faces)
                           (map set))
        normal-data (->> blocks
                         (mapcat block-normal))
        uv-data (->> blocks
                     (map :type)
                     (mapcat t/block-texture))
        side-freqs (frequencies position-data)
        render (->> (interleave position-data normal-data uv-data)
                    (partition 3)
                    (filter (fn [pu] (= 1 (side-freqs (first pu))))))
        indices (flatten
                 (for [x (range (count render))]
                   (map #(+ (* 4 x) %) face-vector)))]
    {:attribs {:position {:data (->> render
                                     (mapcat first)
                                     (flatten)
                                     (arrays/float32))
                          :size 3}

               :normal {:data (->> render
                                   (map second)
                                   (flatten)
                                   (arrays/float32))
                        :size 3}
               :uv {:data (->> render
                               (map last)
                               (flatten)
                               (arrays/float32))
                    :size 2}}
     :indices {:data (arrays/uint16 indices)}
     :num-items (* (count render) 2 3)
     :num-vertices (* (count render) 2 2)
     :mode 4}))

(defn chunkify [world]
  (->> world
       (group-by get-chunk)
       (vals)
       (map chunk-spec)))
