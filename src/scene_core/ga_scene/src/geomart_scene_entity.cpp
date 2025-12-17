#include "geomart_scene_entity.h"

namespace gms = geomart::scene;

gms::GASceneEntity::GASceneEntity(uint32_t id)
	: id_(id)
	, position_(QVector3D(0, 0, 0))
	, rotation_(QQuaternion::fromEulerAngles(0, 0, 0))
	, scale_(QVector3D(1, 1, 1))
{


}

// fake implemetaion
bool gms::GASceneEntity::has_entity_component() const
{
	return false;
}

// fake implemetaion
gms::GASceneEntityType gms::GASceneEntity::get_entity_type() const
{
	return GASceneEntityType();
}

QMatrix4x4 gms::GASceneEntity::get_model_matrix() const
{
	QMatrix4x4 ret_matrix;
	ret_matrix.setToIdentity();
	ret_matrix.translate(position_);
	ret_matrix.rotate(rotation_);
	ret_matrix.scale(scale_);
	return ret_matrix;
}
