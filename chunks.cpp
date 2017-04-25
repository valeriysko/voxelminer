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

// TODO: rewrite chunks.cljs in c++

vector<vector<vector<int>>> getFaces(Vec3 p) {
  // refactor?

  // [[(1 0 1) (1 0 0) (1 1 0) (1 1 1)]
  //  [(0 0 0) (0 0 1) (0 1 1) (0 1 0)]
  //  [(0 1 1) (1 1 1) (1 1 0) (0 1 0)]
  //  [(0 0 0) (1 0 0) (1 0 1) (0 0 1)]
  //  [(0 0 1) (1 0 1) (1 1 1) (0 1 1)]
  //  [(1 0 0) (0 0 0) (0 1 0) (1 1 0)]]

  vector<vector<vector<int>>> faces = {
    {
      {1 + p.x, 0 + p.y, 1 + p.z},
      {1 + p.x, 0 + p.y, 0 + p.z},
      {1 + p.x, 1 + p.y, 0 + p.z},
      {1 + p.x, 1 + p.y, 1 + p.z}
    },
    {
      {0 + p.x, 0 + p.y, 0 + p.z},
      {0 + p.x, 0 + p.y, 1 + p.z},
      {0 + p.x, 1 + p.y, 1 + p.z},
      {0 + p.x, 1 + p.y, 0 + p.z}
    },
    {
      {0 + p.x, 1 + p.y, 1 + p.z},
      {1 + p.x, 1 + p.y, 1 + p.z},
      {1 + p.x, 1 + p.y, 0 + p.z},
      {0 + p.x, 1 + p.y, 0 + p.z}
    },
    {
      {0 + p.x, 0 + p.y, 0 + p.z},
      {1 + p.x, 0 + p.y, 0 + p.z},
      {1 + p.x, 0 + p.y, 1 + p.z},
      {0 + p.x, 0 + p.y, 1 + p.z}
    },
    {
      {0 + p.x, 0 + p.y, 1 + p.z},
      {1 + p.x, 0 + p.y, 1 + p.z},
      {1 + p.x, 1 + p.y, 1 + p.z},
      {0 + p.x, 1 + p.y, 1 + p.z}
    },
    {
      {1 + p.x, 0 + p.y, 0 + p.z},
      {0 + p.x, 0 + p.y, 0 + p.z},
      {0 + p.x, 1 + p.y, 0 + p.z},
      {1 + p.x, 1 + p.y, 0 + p.z}
    }
  };
  return faces;
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

vector<vector<float>> grassTop = {
  {0, 0}, {0.125, 0}, {0.125, 0.125}, {0, 0.125}
};

vector<vector<float>> grassSide = {
  {0.5, 0.125}, {0.375, 0.125}, {0.375, 0}, {0.5, 0}
};

vector<vector<float>> dirtSide = {
  {0.375, 0.125}, {0.25, 0.125}, {0.25, 0}, {0.375, 0}
};

vector<vector<float>> stoneSide = {
  {0.25, 0.125}, {0.125, 0.125}, {0.125, 0}, {0.25, 0}
};

vector<vector<vector<float>>> grassTex = {
  grassSide,
  grassSide,
  grassTop,
  dirtSide,
  grassSide,
  grassSide
};

vector<vector<vector<float>>> stoneTex = {
  stoneSide,
  stoneSide,
  stoneSide,
  stoneSide,
  stoneSide,
  stoneSide
};

vector<vector<vector<float>>> dirtTex = {
  dirtSide,
  dirtSide,
  dirtSide,
  dirtSide,
  dirtSide,
  dirtSide
};

vector<vector<vector<float>>> getTexture(string type) {
  if (type == "grass") {
    return grassTex;
  }
  if (type == "dirt") {
    return dirtTex;
  }
  if (type == "stone") {
    return stoneTex;
  }
  return grassTex;
}

emscripten::val JsArray = emscripten::val::global("Array");
emscripten::val Float32Array = emscripten::val::global("Float32Array");
emscripten::val Uint16Array = emscripten::val::global("Uint16Array");

emscripten::val chunkify(vector<Block> blocks) {
  int blockCount = blocks.size();
  vector<vector<vector<int>>> faces;
  vector<vector<int>> normals;
  vector<vector<vector<float>>> texture;

  vector<set<vector<int>>> facesets;
  map<set<vector<int>>, int> freqs;

  for (Block block: blocks) {
    for (auto face: getFaces(block.position)) {
      faces.push_back(face);
      set<vector<int>> s(face.begin(), face.end());
      facesets.push_back(s);
      freqs[s] = 0;
    }
    for (auto normal: blockNormal) {
      normals.push_back(normal);
    }
    for (auto tex: getTexture(block.type)) {
      texture.push_back(tex);
    }
  }

  for (auto face: facesets) {
    freqs[face] = freqs[face] + 1;
  }

  set<int> blacklist;

  int resultCount = 0;
  for (int i = 0; i < facesets.size(); i++) {
    if (freqs[facesets[i]] == 1) {
      resultCount++;
    } else {
      blacklist.insert(i);
    }
  }

  emscripten::val result = JsArray.new_();

  emscripten::val positionData = Float32Array.new_(resultCount * 4 * 3);
  emscripten::val normalData = Float32Array.new_(resultCount * 4 * 3);
  emscripten::val uvData = Float32Array.new_(resultCount * 4 * 2);
  int p = 0;
  int n = 0;
  int u = 0;
  for (int i = 0; i < faces.size(); i++) {
    if (blacklist.find(i) != blacklist.end()) {
      continue;
    }
    for (auto v: faces[i]) {
      for (int x: v) {
        positionData.set(p, x);
        p++;
      }
    }
    for (int j = 0; j < 4; j++) {
      for (int x: normals[i]) {
        normalData.set(n, x);
        n++;
      }
    }
    for (auto v: texture[i]) {
      for (float f: v) {
        uvData.set(u, f);
        u++;
      }
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
