#pragma once
#include <memory>

#include <QMatrix4x4>

namespace geomart::scene
{
    enum class GASceneEntityType
    {
        Mesh,
        PointCloud,
        BRep,
    };

    // TEMPORARY ADD FOR BUILDING SUCCESSFUL
    // Suppose Data are encapsulated in ISceneDataComponent and its derived classes
    class ISceneDataComponent
    {
    public:
        ISceneDataComponent();
        virtual ~ISceneDataComponent();

        virtual void stand() = 0;
    };

    class GASceneEntity
    {
    public:
        GASceneEntity(uint32_t id);
        ~GASceneEntity() = default;

        template <typename T>
        T* get_data_component()
            requires std::is_base_of_v<ISceneDataComponent, T>
        {
            if constexpr (std::is_same_v<T, ISceneDataComponent>)
            {
                return static_cast<T*>(data_comp_.get());
            }
            else
            {
                T* pComp = dynamic_cast<T*>(data_comp_.get());
                Q_ASSERT_X(nullptr != pComp, "GASceneEntity::getDataComponent",
                    QString("Type mismatch! requested: %1").arg(typeid(T).name()).toStdString().c_str());
                return pComp;
            }
        }

        template <typename T,
            std::enable_if_t<std::is_base_of_v<ISceneDataComponent, T>, int> = 0>
        const T* get_data_component() const
        {
            if constexpr (std::is_same_v<T, ISceneDataComponent>)
            {
                return static_cast<const T*>(data_comp_.get());
            }
            else
            {
                const T* comp = dynamic_cast<const T*>(data_comp_.get());
                Q_ASSERT_X(comp != nullptr, "SceneEntity::getDataComponent", "Type mismatch!");
                return comp;
            }
        }

        // check data invalid
		bool has_entity_component() const;

        //  get data type
		GASceneEntityType get_entity_type() const;

    public:

		// TODO update position, rotation, scale methods

        // M in MVP matrices
        QMatrix4x4 get_model_matrix() const;

    private:
        uint32_t id_;
        std::unique_ptr<ISceneDataComponent> data_comp_;

        QVector3D position_;
        QQuaternion rotation_;
        QVector3D scale_;

        // for unique_ptr
        GASceneEntity(const GASceneEntity&) = delete;
        GASceneEntity& operator=(const GASceneEntity&) = delete;

        // TODO add more states
    };
} // namespace geomart::scene