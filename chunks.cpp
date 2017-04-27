#include <climits>
#include <cmath>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <set>

using namespace std;

struct Vec3 {
  int x, y, z;
};

struct Block {
  Vec3 position;
  string type;
};

const int CHUNK_SIZE = 20;

enum Side : int {
  RIGHT = 0,
  LEFT = 1,
  TOP = 2,
  BOTTOM = 3,
  FRONT= 4,
  BACK = 5
};

vector<int> getFace(Vec3 p, Side side) {
  // refactor?

  // [[(1 0 1) (1 0 0) (1 1 0) (1 1 1)]
  //  [(0 0 0) (0 0 1) (0 1 1) (0 1 0)]
  //  [(0 1 1) (1 1 1) (1 1 0) (0 1 0)]
  //  [(0 0 0) (1 0 0) (1 0 1) (0 0 1)]
  //  [(0 0 1) (1 0 1) (1 1 1) (0 1 1)]
  //  [(1 0 0) (0 0 0) (0 1 0) (1 1 0)]]

  switch(side) {
  case RIGHT:
    return { // right
      1 + p.x, 0 + p.y, 1 + p.z,
      1 + p.x, 0 + p.y, 0 + p.z,
      1 + p.x, 1 + p.y, 0 + p.z,
      1 + p.x, 1 + p.y, 1 + p.z
    };
  case LEFT:
    return { // left
      0 + p.x, 0 + p.y, 0 + p.z,
      0 + p.x, 0 + p.y, 1 + p.z,
      0 + p.x, 1 + p.y, 1 + p.z,
      0 + p.x, 1 + p.y, 0 + p.z
    };
  case TOP:
    return { // top +[0 1 0]
      0 + p.x, 1 + p.y, 1 + p.z,
      1 + p.x, 1 + p.y, 1 + p.z,
      1 + p.x, 1 + p.y, 0 + p.z,
      0 + p.x, 1 + p.y, 0 + p.z
    };
  case BOTTOM:
    return { // bottom +[0 -1 0]
      0 + p.x, 0 + p.y, 0 + p.z,
      1 + p.x, 0 + p.y, 0 + p.z,
      1 + p.x, 0 + p.y, 1 + p.z,
      0 + p.x, 0 + p.y, 1 + p.z
    };
  case FRONT:
    return { // front
      0 + p.x, 0 + p.y, 1 + p.z,
      1 + p.x, 0 + p.y, 1 + p.z,
      1 + p.x, 1 + p.y, 1 + p.z,
      0 + p.x, 1 + p.y, 1 + p.z
    };
  default:
    return { // back
      1 + p.x, 0 + p.y, 0 + p.z,
      0 + p.x, 0 + p.y, 0 + p.z,
      0 + p.x, 1 + p.y, 0 + p.z,
      1 + p.x, 1 + p.y, 0 + p.z
    };
  }
}

vector<vector<int>> blockNormal =
  {
    {1, 0, 0},
    {-1, 0, 0},
    {0, 1, 0},
    {0, -1, 0},
    {0, 0, 1},
    {0, 0, -1}
  };

vector<float> grassTop = {
  0, 0, 0.125, 0, 0.125, 0.125, 0, 0.125
};

vector<float> grassSide = {
  0.5, 0.125, 0.375, 0.125, 0.375, 0, 0.5, 0
};

vector<float> dirtSide = {
  0.375, 0.125, 0.25, 0.125, 0.25, 0, 0.375, 0
};

vector<float> stoneSide = {
  0.25, 0.125, 0.125, 0.125, 0.125, 0, 0.25, 0
};

vector<vector<float>> grassTex = {
  grassSide,
  grassSide,
  grassTop,
  dirtSide,
  grassSide,
  grassSide
};

vector<vector<float>> stoneTex = {
  stoneSide,
  stoneSide,
  stoneSide,
  stoneSide,
  stoneSide,
  stoneSide
};

vector<vector<float>> dirtTex = {
  dirtSide,
  dirtSide,
  dirtSide,
  dirtSide,
  dirtSide,
  dirtSide
};

vector<float> getTexture(string type, Side side) {
  if (type == "grass") {
    return grassTex[side];
  }
  if (type == "dirt") {
    return dirtTex[side];
  }
  if (type == "stone") {
    return stoneTex[side];
  }
  return grassTex[side];
}

emscripten::val JsArray = emscripten::val::global("Array");
emscripten::val Float32Array = emscripten::val::global("Float32Array");
emscripten::val Uint16Array = emscripten::val::global("Uint16Array");

bool shouldRender(bool ***chunk, int x, int y, int z) {
  return !chunk[abs(x) % CHUNK_SIZE][abs(y) % CHUNK_SIZE][abs(z) % CHUNK_SIZE];
}

emscripten::val chunkify(vector<Block> blocks) {
  bool ***chunk = new bool**[CHUNK_SIZE];
  for (int i = 0; i < CHUNK_SIZE; i++) {
    chunk[i] = new bool*[CHUNK_SIZE];
    for (int j = 0; j < CHUNK_SIZE; j++) {
      chunk[i][j] = new bool[CHUNK_SIZE];
      for (int k = 0; k < CHUNK_SIZE; k++) {
        chunk[i][j][k] = false;
      }
    }
  }

  int blockCount = blocks.size();
  vector<vector<int>> faces;
  vector<vector<int>> normals;
  vector<vector<float>> texture;

  int xMin = INT_MAX;
  int xMax = INT_MIN;
  int yMin = INT_MAX;
  int yMax = INT_MIN;
  int zMin = INT_MAX;
  int zMax = INT_MIN;
  for (Block block: blocks) {
    Vec3 pos = block.position;
    chunk[abs(pos.x) % CHUNK_SIZE][abs(pos.y) % CHUNK_SIZE][abs(pos.z) % CHUNK_SIZE] = true;
    xMin = min(xMin, pos.x);
    xMax = max(xMax, pos.x);
    yMin = min(yMin, pos.y);
    yMax = max(yMax, pos.y);
    zMin = min(zMin, pos.z);
    zMax = max(zMax, pos.z);
  }

  for (Block block: blocks) {
    Vec3 pos = block.position;
    int x = pos.x;
    int y = pos.y;
    int z = pos.z;

    if (x + 1 > xMax || shouldRender(chunk, x + 1, y, z)) {
      faces.push_back(getFace(pos, RIGHT));
      normals.push_back(blockNormal[RIGHT]);
      texture.push_back(getTexture(block.type, RIGHT));
    }
    if (x - 1 < xMin || shouldRender(chunk, x - 1, y, z)) {
      faces.push_back(getFace(pos, LEFT));
      normals.push_back(blockNormal[LEFT]);
      texture.push_back(getTexture(block.type, LEFT));
    }
    if (y + 1 > yMax || shouldRender(chunk, x, y + 1, z)) {
      faces.push_back(getFace(pos, TOP));
      normals.push_back(blockNormal[TOP]);
      texture.push_back(getTexture(block.type, TOP));
    }
    if (y - 1 < yMin || shouldRender(chunk, x, y - 1, z)) {
      faces.push_back(getFace(pos, BOTTOM));
      normals.push_back(blockNormal[BOTTOM]);
      texture.push_back(getTexture(block.type, BOTTOM));
    }
    if (z + 1 > zMax || shouldRender(chunk, x, y, z + 1)) {
      faces.push_back(getFace(pos, FRONT));
      normals.push_back(blockNormal[FRONT]);
      texture.push_back(getTexture(block.type, FRONT));
    }
    if (z - 1 < yMin || shouldRender(chunk, x, y, z - 1)) {
      faces.push_back(getFace(pos, BACK));
      normals.push_back(blockNormal[BACK]);
      texture.push_back(getTexture(block.type, BACK));
    }
  }

  int resultCount = faces.size();
  emscripten::val result = JsArray.new_();

  emscripten::val positionData = Float32Array.new_(resultCount * 4 * 3);
  emscripten::val normalData = Float32Array.new_(resultCount * 4 * 3);
  emscripten::val uvData = Float32Array.new_(resultCount * 4 * 2);
  int p = 0;
  int n = 0;
  int u = 0;
  for (int i = 0; i < faces.size(); i++) {
    for (int x: faces[i]) {
      positionData.set(p, x);
      p++;
    }
    for (int j = 0; j < 4; j++) {
      for (int x: normals[i]) {
        normalData.set(n, x);
        n++;
      }
    }
    for (float f: texture[i]) {
      uvData.set(u, f);
      u++;
    }
  }

  emscripten::val indices = Uint16Array.new_(resultCount * 6);
  for (int i = 0; i < n; i++) {
    int m = i * 4;
    indices.set(i*6, 0 + m);
    indices.set(i*6+1, 1 + m);
    indices.set(i*6+2, 2 + m);
    indices.set(i*6+3, 0 + m);
    indices.set(i*6+4, 2 + m);
    indices.set(i*6+5, 3 + m);
  }

  result.set(0, resultCount);
  result.set(1, positionData);
  result.set(2, normalData);
  result.set(3, uvData);
  result.set(4, indices);

  for (int i = 0; i < CHUNK_SIZE; i++) {
    for (int j = 0; j < CHUNK_SIZE; j++) {
      delete[] chunk[i][j];
    }
    delete[] chunk[i];
  }
  delete[] chunk;

  return result;
}

EMSCRIPTEN_BINDINGS(my_module) {
  emscripten::value_array<Vec3>("Vec3")
    .element(&Vec3::x)
    .element(&Vec3::y)
    .element(&Vec3::z);

  emscripten::value_object<Block>("Block")
    .field("position", &Block::position)
    .field("type", &Block::type);

  emscripten::register_vector<Block>("VectorBlock");

  emscripten::function("chunkify", &chunkify);
}
