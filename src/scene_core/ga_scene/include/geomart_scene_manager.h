#pragma once

#include "geomart_scene_entity.h"

#include <map>
#include <memory>
#include <vector>

namespace geomart::scene
{
    class GASceneEntity;
    class GASceneManager
    {
    public:
        // for singleton and disable copy/move
        static GASceneManager& instance()
        {
            static GASceneManager instance;
			return instance;
        }
        ~GASceneManager() = default;

        GASceneManager(const GASceneManager &) = delete;
        GASceneManager &operator=(const GASceneManager &) = delete;

        GASceneManager(const GASceneManager &&) = delete;
        GASceneManager &operator=(const GASceneManager &&) = delete;

    public:
        // member functions for entity management

        // create a scene entity and return its pointer for data filling
        GASceneEntity *create_scene_entity();

        // get scene entity by id
        GASceneEntity *get_scene_entity(uint32_t id) const;

        // remove scene entity by id
        bool remove_scene_entity(uint32_t id);

        // get certain type of scene entities
		std::vector<GASceneEntity*> get_scene_entites_by_type(GASceneEntityType type);
        
        // get all scene entities
        const std::vector<std::unique_ptr<GASceneEntity>> &get_all_scene_entities() const;

		void remove_all_entities();

    private:
        // private constructor for singleton
        GASceneManager();

    private:
        // member variables

        // all scene entities, UNIQUE, cannot assume the index is continuous and ascending
        std::vector<std::unique_ptr<GASceneEntity>> scene_entities_;

        // next available index
        uint32_t next_index_;

        // for quick access, and maybe revise, so remove const property
        std::map<uint32_t, GASceneEntity *> index_entity_map_;

        // TODO: other members
    };
} // namespace geomart::scene