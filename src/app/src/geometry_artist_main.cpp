#include "geometry_artist_main.h"

geomart::GeometryArtistMain::GeometryArtistMain(QWidget *parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
{
	ui_ = new Ui_Geometry_artist_mainwindow();
	ui_->setupUi(this);

	treatGUI();
	treatMenu();
	treatAction();
}

geomart::GeometryArtistMain::~GeometryArtistMain()
{
	delete ui_;
	ui_ = nullptr;
}

void geomart::GeometryArtistMain::treatGUI()
{
	// TODO
	/*scene_manager_ = std::make_unique<GeomScene>();
	viewer_manager_ = std::make_unique<GeomViewer>(this);
	viewer_manager_->setScene(scene_manager_.get());
	ui_->verticalLayout->addWidget(viewer_manager_.get());*/
}

void geomart::GeometryArtistMain::treatMenu()
{
	// TODO
}

void geomart::GeometryArtistMain::treatAction()
{
	actgrpShading_ = new QActionGroup(this);
	actgrpShading_->addAction(ui_->actionFlatShading);
	actgrpShading_->addAction(ui_->actionGouraudShading);
	ui_->actionFlatShading->setChecked(true);
	ui_->actionGouraudShading->setChecked(true);
}