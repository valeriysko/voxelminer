#include <stdio.h>
#include <emscripten/bind.h>

using namespace emscripten;

struct Position {
  int x, y, z;
};

struct Block {
  Position position;
  std::string type;
};

// TODO: rewrite chunks.cljs in c++

int chunkify(std::vector<Block> blocks) {
  return blocks.size();
}

EMSCRIPTEN_BINDINGS(my_module) {
  value_array<Position>("Position")
    .element(&Position::x)
    .element(&Position::y)
    .element(&Position::z);

  value_object<Block>("Block")
    .field("position", &Block::position)
    .field("type", &Block::type);

  register_vector<Block>("VectorBlock");

  function("chunkify", &chunkify);
}
