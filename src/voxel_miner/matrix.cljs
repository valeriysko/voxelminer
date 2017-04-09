(ns voxel-miner.matrix
  (:require [clojure.core.matrix :as matrix]))

(def i4 (matrix/identity-matrix 4))

(defn rotate-x [m angle]
  (let [c (.cos js/Math angle)
        s (.sin js/Math angle)]
    (matrix/mmul [[1 0 0 0]
                  [0 c s 0]
                  [0 (- 0 s) c 0]
                  [0 0 0 1]]
                 m)))

(defn rotate-y [m angle]
  (let [c (.cos js/Math angle)
        s (.sin js/Math angle)]
    (matrix/mmul [[c 0 (- 0 s) 0]
                  [0 1 0 0]
                  [s 0 c 0]
                  [0 0 0 1]]
                 m)))

(defn rotate-z [m angle]
  (let [c (.cos js/Math angle)
        s (.sin js/Math angle)]
    (matrix/mmul [[c s 0 0]
                  [(- 0 s) c 0 0]
                  [0 0 1 0]
                  [0 0 0 1]]
                 m)))

(defn translate-x [m t]
  (matrix/add [[0 0 0 0]
               [0 0 0 0]
               [0 0 0 0]
               [t 0 0 0]]
              m))

(defn translate-y [m t]
  (matrix/add [[0 0 0 0]
               [0 0 0 0]
               [0 0 0 0]
               [0 t 0 0]]
              m))

(defn translate-z [m t]
  (matrix/add [[0 0 0 0]
               [0 0 0 0]
               [0 0 0 0]
               [0 0 t 0]]
              m))

(defn set-position [m [x y z]]
  (-> m
      (assoc-in [3 0] x)
      (assoc-in [3 1] y)
      (assoc-in [3 2] z)))
