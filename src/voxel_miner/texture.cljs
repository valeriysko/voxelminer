(ns voxel-miner.texture)

(def texture-file "/texture.png")

(defn block-texture [type]
  (def grass-top [[0 0] [0.125 0] [0.125 0.125] [0 0.125]])
  (def grass-side [[0.5 0.125] [0.375 0.125] [0.375 0] [0.5 0]])
  (def dirt [[0.375 0.125] [0.25 0.125] [0.25 0] [0.375 0]])
  (def stone [[0.25 0.125] [0.125 0.125] [0.125 0] [0.25 0]])
  (case type
    :grass [grass-side
            grass-side
            grass-top
            dirt
            grass-side
            grass-side]
    :dirt (repeat 6 dirt)
    :stone (repeat 6 stone)))
