#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    cam.dev_open();
    cam.dev_init();
    cam.init_view();
    cam.run_SDL_SetEventFilter();;
    cam.start_capturing();
    cam.mainloop();
}

MainWindow::~MainWindow()
{
    delete ui;

}
