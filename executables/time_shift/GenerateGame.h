#pragma once

#include <filesystem>
#include <json.hpp>

#include "Transform.hpp"

struct Sprite
{
  std::string name;
  std::vector<glm::vec2> uvs;
  MFA::Transform *transform_ptr;
};

class GenerateGame {
public:

  explicit GenerateGame(const std::filesystem::path &json_path);

  [[nodiscard]]
  std::vector<std::shared_ptr<MFA::Transform>> const & Transforms() const {return transforms;}

  [[nodiscard]]
  std::vector<std::shared_ptr<Sprite>> const & Sprites() const {return sprites;}

private:
  void parse_objects(MFA::Transform* parent, nlohmann::json objects);

  std::vector<std::shared_ptr<MFA::Transform>> transforms;
  std::vector<std::shared_ptr<Sprite>> sprites;
};