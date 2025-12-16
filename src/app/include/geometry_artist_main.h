#pragma once

#include <memory>

#include <QObject>
#include <QMainWindow>
#include <QActionGroup>

#include "main_window/ui_geometry_artist_main.h"

class QActionGroup;
class Ui_Geometry_artist_mainwindow;

namespace geomart
{
	class BooleanScene;
	class BooleanViewer;
	class GeometryArtistMain : public QMainWindow
	{
		Q_OBJECT

	public:
		explicit GeometryArtistMain(QWidget *parent = nullptr, Qt::WindowFlags flags = {});
		virtual ~GeometryArtistMain() override;

		GeometryArtistMain(const GeometryArtistMain &) = delete;
		GeometryArtistMain &operator=(const GeometryArtistMain &) = delete;
		GeometryArtistMain(GeometryArtistMain &&) = delete;
		GeometryArtistMain &operator=(GeometryArtistMain &&) = delete;

	public Q_SLOTS:

	private:
		void treatGUI();
		void treatMenu();
		void treatAction();

	private:
		// TODO
		// std::unique_ptr<GeomScene> scene_manager_;
		// std::unique_ptr<GeomViewer> viewer_manager_;

	private:
		Ui_Geometry_artist_mainwindow *ui_ = nullptr;
		QActionGroup *actgrpShading_ = nullptr;
	};
}
