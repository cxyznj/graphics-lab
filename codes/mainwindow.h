#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QDebug>
#include <QVector>
#include <QMouseEvent>
#include <QPen>
#include <assert.h>
#include <QColorDialog>
#include <algorithm>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QScreen>
#include <QGuiApplication>
#include <QProcess>

#define PI 3.1415926535897932

enum Figurestate {Free, Line, Circle, Ellipse, Rectangle, Polygon, QuadBe, CubicBe};

struct Figures{
    enum Figurestate state;
    union Fu{
        struct { int x1; int y1; int x2; int y2; } line;
        struct { int cx; int cy; int r; } circle;
        struct { int ex; int ey; int rx; int ry; } ellipse;
        struct { int x; int y; int w; int h; } rectangle;
        struct { int x[200]; int y[200]; int count; } polygon;
        struct { int x[3]; int y[3]; int count; } quadbe;
        struct { int x[4]; int y[4]; int count; } cubicbe;
    } content;
    bool choose = false;
    bool color = false;
    QColor thiscolor;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    // 调用draw_figures()
    void paintEvent(QPaintEvent *event);

    void mousePressEvent(QMouseEvent *event);

    void mouseReleaseEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event);

private slots:
    void on_action_DrawLine_triggered();

    void on_action_DrawCircle_triggered();

    void on_action_DrawEllipse_triggered();

    void on_action_DrawRectangle_triggered();

    void on_actionPolygon_triggered();

    void on_action_Undo_triggered();

    void on_action_remove_triggered();

    void on_actionColor_triggered();

    void on_actionChange_Color_triggered();

    void on_action_Save_triggered();

    void on_action_Load_triggered();

    void on_action_Quadratic_B_zier_curves_triggered();

    void on_action_Cubic_B_zier_curves_triggered();

    void on_action_Cut_triggered();

    void on_actionOpen3D_triggered();

    void on_actionInitialize_triggered();

    void on_actionCancel_All_States_triggered();

    void on_actionHow_to_use_triggered();

private:
    Ui::MainWindow *ui;
    // 主画笔
    QPainter* mainpainter;
    // 状态集合：
    // 1:inpainting
    // 2:inchanging
    // 3:inmoving
    // 4:inremoving
    // 5:incoloring
    // 6:begin to paint
    // 7:rotating
    // 8:begin to cut
    // 9:cutting
    int curstate;
    // 绘画图形
    enum Figurestate statepainter;
    // 变化图形
    enum Figurestate statechange;
    // 被绘制的图形集
    QVector<Figures> figures;
    QVector<Figures> oldfigures;
    // 绘制的起始位置
    int pstartx, pstarty;
    // 绘制的终止位置
    int pendx, pendy;
    // 变换的起始位置
    int mstartx, mstarty;
    // 填充颜色
    QColor curcolor;

    // 绘制图形集
    void draw_figures();
    // 根据pstartx, pstarty, pendx, pendy和statepainter为figures添加一个figure
    void add_figure(int state);
    // 删除尾部的figure
    void remove_figure();
    // 高亮被点中的图形
    bool choosefigure(int clix, int cliy);
    // 删除被点中的图形
    bool removefigure(int clix, int cliy);
    // 判断是否用户点击了图形变化区域，1表示调整，2表示平移
    int changefigure(int clix, int cliy);
    // 对第一个点中的图形改变其染色状态
    int colorfigure(int clix, int cliy);
    // 区域染色:边界填充算法
    void fillcolor(Figures f, QColor c, QPoint p0);
    bool point_in_edge(Figures f, QPoint p);
    void fillleft(Figures f, QPoint p0);
    void fillup(Figures f, QPoint p0);
    void fillright(Figures f, QPoint p0);
    void filldown(Figures f, QPoint p0);

    // 判断一个点是否在一个图形（多边形）内部
    bool point_in_figure(Figures f, int x0, int y0);

    // 选取裁剪图形
    void select_clipper();
    // 裁剪算法
    void liang_barsky_clipper(float xmin, float ymin, float xmax, float ymax,
                              float x1, float y1, float x2, float y2, int fno);
    void Sutherland_Hodgman_clipper(float xmin, float ymin, float xmax, float ymax, int fno);

    // 绘制算法
    void paint_line(int x1, int y1, int x2, int y2, bool bechoose);
    void paint_circle(int x, int y, int r, bool bechoose, bool becolor, QColor col);
    void paint_ellipse(int x, int y, int rx, int ry, bool bechoose, bool becolor, QColor col);
    void paint_rectangle(int x1, int y1, int x2, int y2, bool bechoose, bool becolor, QColor col);
    void paint_polygon(int x[], int y[], int num, bool bechoose, bool becolor, QColor col);
};

#endif // MAINWINDOW_H
