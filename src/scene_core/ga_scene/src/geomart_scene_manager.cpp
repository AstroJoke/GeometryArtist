#include "geomart_scene_manager.h"

namespace gms = geomart::scene;

gms::GASceneEntity* gms::GASceneManager::create_scene_entity()
{
	uint32_t new_id = next_index_++;
	auto entity = std::make_unique<GASceneEntity>(new_id);
	gms::GASceneEntity* raw_entity_ptr = entity.get();
	scene_entities_.emplace_back(std::move(entity));
	index_entity_map_[new_id] = raw_entity_ptr;
	return raw_entity_ptr;
}

gms::GASceneEntity* gms::GASceneManager::get_scene_entity(uint32_t id) const
{
    auto it = index_entity_map_.find(id);
    if (it == index_entity_map_.end())
        return nullptr;
    else
		return it->second;
}

bool gms::GASceneManager::remove_scene_entity(uint32_t id)
{
    // find
    auto it = index_entity_map_.find(id);
    if (it == index_entity_map_.end())
        return false;

    gms::GASceneEntity* raw_entity = it->second;
    auto vec_it = std::find_if(scene_entities_.begin(), scene_entities_.end(),
        [raw_entity](const std::unique_ptr<gms::GASceneEntity>& ptr) {
            return raw_entity == ptr.get();
        });

    if (vec_it != scene_entities_.end())
    {
        scene_entities_.erase(vec_it);
        index_entity_map_.erase(it);
        return true;
    }
    return false;
}

std::vector<gms::GASceneEntity*> gms::GASceneManager::get_scene_entites_by_type(GASceneEntityType type)
{
    std::vector<GASceneEntity*> result;
    for (const auto& entity_ptr : scene_entities_)
    {
        if (entity_ptr->has_entity_component() && type == entity_ptr->get_entity_type())
        {
            result.push_back(entity_ptr.get());
        }
    }
    return result;
}

const std::vector<std::unique_ptr<gms::GASceneEntity>>& gms::GASceneManager::get_all_scene_entities() const
{
    return scene_entities_;
}

void gms::GASceneManager::remove_all_entities()
{
    scene_entities_.clear();
	index_entity_map_.clear();
	next_index_ = 0;
}

gms::GASceneManager::GASceneManager()
	: next_index_(0)
{

}
