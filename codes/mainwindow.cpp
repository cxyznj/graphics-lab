#include "mainwindow.h"
#include "ui_mainwindow.h"

// 为区域填充准备的变量
QPoint fill_point_list[1000000]; int fill_point_count;
Figures fillfigure;

// 计算扫描线与多边形所有的交点，奇数交点到偶数交点间为填充点（如果起始点在多边形外的话）
QPoint intersect_point_list[500]; int intersect_point_count;

// 记录当前改变的图形（和多边形或曲线的结点）
int change_no = 0;
int polygon_point_no = 0;

// 为椭圆和矩形旋转设置的变量，保证这两个图形在第一次进入指定区域时发生变化
bool rotate_ellipse_rectangle;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->resize(800, 600);
    setWindowTitle(tr("Drawing Board"));
    // 变量的初始化
    mainpainter = new QPainter(this);
    statepainter = Free;
    statechange = Free;
    curstate = 0;

    // 捕获鼠标的激活
    setMouseTracking(true);
    ui->centralWidget->setMouseTracking(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *)
{
    draw_figures();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        // 对点中的图形进行高亮，choose属性设为true
        choosefigure(event->pos().x(), event->pos().y());

        // 当前状态是图形删除状态，对选中的图形进行删除
        if(curstate == 4)
        {
            removefigure(event->pos().x(), event->pos().y());
        }
        // 当前状态是染色状态，对一个图形进行染色
        else if(curstate == 5)
        {
            colorfigure(event->pos().x(), event->pos().y());
        }
        // 当前状态是绘制的开始状态
        else if(curstate == 6)
        {
            // 绘制一个新图形
            if(statepainter != Free)
            {
                pstartx = pendx = event->pos().x();
                pstarty = pendy = event->pos().y();
                curstate = 1;
                if(statepainter != Polygon && statepainter != QuadBe && statepainter != CubicBe)
                {
                    add_figure(statepainter);
                }
                else
                {
                    // 当绘制的是多边形或贝塞尔曲线时，只考虑鼠标松开的位置
                    ;
                }

            }
        }
        else if(curstate == 8)
        {
            pstartx = pendx = event->pos().x();
            pstarty = pendy = event->pos().y();
            curstate = 9;
            add_figure(Rectangle);
        }
        // 判断是否点到了图形的变换点（平移点），且当前不是绘制状态（否则有可能边画边变）
        else if(curstate != 1)
        {
            int cf = changefigure(event->pos().x(), event->pos().y());
            // 鼠标点击图形的变换点
            if(cf == 1)
            {
                // pstartx 和pstarty已由changefigure设置为图形的另一边的端点
                oldfigures = figures;
                curstate = 2;
            }
            // 鼠标点击图形的平移点
            else if(cf == 2)
            {
                // mstartx 和mstarty同理已设好
                oldfigures = figures;
                curstate = 3;
            }
            // 鼠标点击图形的旋转点
            else if(cf == 3)
            {
                oldfigures = figures;
                curstate = 7;
            }
        }
    }

    // 右键还原状态
    else if(event->button() == Qt::RightButton)
    {
        statepainter = Free;
        statechange = Free;
        curstate = 0;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 绘图中
    if(curstate == 1)
    {
        if(statepainter != Polygon && statepainter != QuadBe && statepainter != CubicBe)
        {
            remove_figure();
            pendx = event->pos().x();
            pendy = event->pos().y();
            add_figure(statepainter);
            this->update();
        }
        else
        {
            // TODO: 让多边形的绘制更动态（没有也行）
        }
    }
    // 变换中
    else if(curstate == 2)
    {
        if(statechange == Line)
        {
            pendx = event->pos().x();
            pendy = event->pos().y();

            // 坐标的规范化
            int regsx = pstartx, regsy = pstarty, regex = pendx, regey = pendy;
            if(pstartx > pendx)
            {
                regsx = pendx;
                regsy = pendy;
                regex = pstartx;
                regey = pstarty;
            }

            figures[change_no].content.line.x1 = regsx;
            figures[change_no].content.line.y1 = regsy;
            figures[change_no].content.line.x2 = regex;
            figures[change_no].content.line.y2 = regey;
            this->update();
        }
        else if(statechange == Ellipse)
        {
            pendx = event->pos().x();
            pendy = event->pos().y();
            figures[change_no].content.ellipse.ex = (pstartx + pendx)/2;
            figures[change_no].content.ellipse.ey = (pstarty + pendy)/2;
            figures[change_no].content.ellipse.rx = abs(pendx - pstartx)/2;
            figures[change_no].content.ellipse.ry = abs(pendy - pstarty)/2;
            this->update();
        }
        else if(statechange == Rectangle)
        {
            pendx = event->pos().x();
            pendy = event->pos().y();

            // 坐标的规范化
            int regsx = pstartx, regsy = pstarty, regex = pendx, regey = pendy;
            if(pstartx < pendx)
            {
                // 左上
                if(pstarty < pendy) ;
                // 左下
                else
                {
                    regsy = pendy;
                    regey = pstarty;
                }
            }
            else
            {
                // 右上
                if(pstarty < pendy)
                {
                    regsx = pendx;
                    regex = pstartx;
                }
                // 右下
                else
                {
                    regsx = pendx;
                    regsy = pendy;
                    regex = pstartx;
                    regey = pstarty;
                }
            }
            figures[change_no].content.rectangle.x = regsx;
            figures[change_no].content.rectangle.y = regsy;
            figures[change_no].content.rectangle.w = abs(regex - regsx);
            figures[change_no].content.rectangle.h = abs(regey - regsy);
            this->update();
        }
        else if(statechange == Circle)
        {
            // 计算偏移量
            int dy = -(event->pos().y() - mstarty);
            figures[change_no].content.circle.r += dy;

            // 半径的最小值为0，不能无限缩小
            if(figures[change_no].content.circle.r < 0) figures[change_no].content.circle.r = 0;
            mstartx = event->pos().x();
            mstarty = event->pos().y();
            this->update();
        }
        else if(statechange == Polygon)
        {
            figures[change_no].content.polygon.x[polygon_point_no] = event->pos().x();
            figures[change_no].content.polygon.y[polygon_point_no] = event->pos().y();
            if(polygon_point_no == 0)
            {
                int count = figures[change_no].content.polygon.count;
                figures[change_no].content.polygon.x[count-1] = event->pos().x();
                figures[change_no].content.polygon.y[count-1] = event->pos().y();
            }
            this->update();
        }
        else if(statechange == QuadBe)
        {
            figures[change_no].content.quadbe.x[polygon_point_no] = event->pos().x();
            figures[change_no].content.quadbe.y[polygon_point_no] = event->pos().y();
            this->update();
        }
        else if(statechange == CubicBe)
        {
            figures[change_no].content.cubicbe.x[polygon_point_no] = event->pos().x();
            figures[change_no].content.cubicbe.y[polygon_point_no] = event->pos().y();
            this->update();
        }
    }
    // 平移中
    else if(curstate == 3)
    {
        if(statechange == Line)
        {
            int dx = event->pos().x() - mstartx;
            int dy = event->pos().y() - mstarty;
            figures[change_no].content.line.x1 += dx;
            figures[change_no].content.line.y1 += dy;
            figures[change_no].content.line.x2 += dx;
            figures[change_no].content.line.y2 += dy;
            mstartx = event->pos().x();
            mstarty = event->pos().y();
            this->update();
        }
        else if(statechange == Circle)
        {
            int dx = event->pos().x() - mstartx;
            int dy = event->pos().y() - mstarty;
            figures[change_no].content.circle.cx += dx;
            figures[change_no].content.circle.cy += dy;
            mstartx = event->pos().x();
            mstarty = event->pos().y();
            this->update();
        }
        else if(statechange == Ellipse)
        {
            int dx = event->pos().x() - mstartx;
            int dy = event->pos().y() - mstarty;
            figures[change_no].content.ellipse.ex += dx;
            figures[change_no].content.ellipse.ey += dy;
            mstartx = event->pos().x();
            mstarty = event->pos().y();
            this->update();
        }
        else if(statechange == Rectangle)
        {
            int dx = event->pos().x() - mstartx;
            int dy = event->pos().y() - mstarty;
            figures[change_no].content.rectangle.x += dx;
            figures[change_no].content.rectangle.y += dy;
            mstartx = event->pos().x();
            mstarty = event->pos().y();
            this->update();
        }
        else if(statechange == Polygon)
        {
            int dx = event->pos().x() - mstartx;
            int dy = event->pos().y() - mstarty;
            int count = figures[change_no].content.polygon.count;
            for(int i = 0; i < count; i++)
            {
                figures[change_no].content.polygon.x[i] += dx;
                figures[change_no].content.polygon.y[i] += dy;
            }
            mstartx = event->pos().x();
            mstarty = event->pos().y();
            this->update();
        }
    }
    else if(curstate == 7)
    {
        int mendx = event->pos().x();
        int mendy = event->pos().y();

        // 计算旋转角
        double sita = 0;
        if(mendx == mstartx)
        {
            if(mendy <= mstarty)
                sita = 0;
            else
                sita = PI/2;
        }
        else if(mendy == mstarty)
        {
            if(mendx >= mstartx)
                sita = PI/4;
            else
                sita = PI * 3 / 2;
        }
        else
        {
            double deltax = abs(mstartx - mendx);
            double deltay = abs(mstarty - mendy);
            sita = atan(deltax / deltay);

            // 不同象限的条件判断
            if(mendx > mstartx && mendy > mstarty)
            {
                sita = PI - sita;
            }
            else if(mendx < mstartx && mendy > mstarty)
            {
                sita = PI + sita;
            }
            else if(mendx < mstartx && mendy < mstarty)
            {
                sita = 2 * PI - sita;
            }
        }
        // qDebug() << sita * 180 / PI;
        // 调整旋转速度
        sita /= 90;

        if(statechange == Line)
        {
            // 根据旋转角改变坐标
            assert(figures[change_no].state == Line);
            int x1 = figures[change_no].content.line.x1 - mstartx;
            int y1 = figures[change_no].content.line.y1 - mstarty;
            int x2 = figures[change_no].content.line.x2 - mstartx;
            int y2 = figures[change_no].content.line.y2 - mstarty;
            figures[change_no].content.line.x1 = x1 * cos(sita) - y1 * sin(sita) + mstartx + 0.5;
            figures[change_no].content.line.y1 = x1 * sin(sita) + y1 * cos(sita) + mstarty + 0.5;
            figures[change_no].content.line.x2 = x2 * cos(sita) - y2 * sin(sita) + mstartx + 0.5;
            figures[change_no].content.line.y2 = x2 * sin(sita) + y2 * cos(sita) + mstarty + 0.5;

            // 规范化
            if(figures[change_no].content.line.x1 > figures[change_no].content.line.x2)
            {
                figures[change_no].content.line.x2 = x1 * cos(sita) - y1 * sin(sita) + mstartx + 0.5;
                figures[change_no].content.line.y2 = x1 * sin(sita) + y1 * cos(sita) + mstarty + 0.5;
                figures[change_no].content.line.x1 = x2 * cos(sita) - y2 * sin(sita) + mstartx + 0.5;
                figures[change_no].content.line.y1 = x2 * sin(sita) + y2 * cos(sita) + mstarty + 0.5;
            }

            this->update();
        }
        else if(statechange == Polygon)
        {
            assert(figures[change_no].state == Polygon);
            for(int i = 0; i < figures[change_no].content.polygon.count; i++)
            {
                int x1 = figures[change_no].content.polygon.x[i] - mstartx;
                int y1 = figures[change_no].content.polygon.y[i] - mstarty;
                figures[change_no].content.polygon.x[i] = x1 * cos(sita) - y1 * sin(sita) + mstartx + 0.5;
                figures[change_no].content.polygon.y[i] = x1 * sin(sita) + y1 * cos(sita) + mstarty + 0.5;
            }
            this->update();
        }
        else if(statechange == Ellipse)
        {
            double angle = sita * 90 * 180/ PI;
            if((90 <= angle && angle <= 180) || (270 <= angle && angle <= 360))
            {
                if(!rotate_ellipse_rectangle)
                {
                    int t1 = figures[change_no].content.ellipse.rx;
                    figures[change_no].content.ellipse.rx = figures[change_no].content.ellipse.ry;
                    figures[change_no].content.ellipse.ry = t1;
                    rotate_ellipse_rectangle = true;
                    this->update();
                }
            }
            else
            {
                rotate_ellipse_rectangle = false;
            }
        }
        else if(statechange == Rectangle)
        {
            double angle = sita * 90 * 180/ PI;
            if((90 <= angle && angle <= 180) || (270 <= angle && angle <= 360))
            {
                if(!rotate_ellipse_rectangle)
                {
                    int ex = figures[change_no].content.rectangle.x;
                    int ey = figures[change_no].content.rectangle.y;
                    double w = figures[change_no].content.rectangle.w;
                    double h = figures[change_no].content.rectangle.h;
                    figures[change_no].content.rectangle.x = ex + (w - h)/2 + 0.5;
                    figures[change_no].content.rectangle.y = ey + (h - w)/2 + 0.5;
                    figures[change_no].content.rectangle.h = w;
                    figures[change_no].content.rectangle.w = h;
                    rotate_ellipse_rectangle = true;
                    this->update();
                }
            }
            else
            {
                rotate_ellipse_rectangle = false;
            }
        }
        else
        {
            assert(0);
        }
    }
    else if(curstate == 9)
    {
        remove_figure();
        pendx = event->pos().x();
        pendy = event->pos().y();
        add_figure(Rectangle);
        this->update();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        // 结束一个图形的绘制
        if(curstate == 1)
        {
            pendx = event->pos().x();
            pendy = event->pos().y();
            if(statepainter != Polygon && statepainter != QuadBe && statepainter != CubicBe)
            {
                curstate = 0;
                remove_figure();
                add_figure(statepainter);
            }
            else if(statepainter == Polygon)
            {
                // 鼠标释放时落下多边形的一个点
                add_figure(statepainter);
            }
            else if(statepainter == QuadBe)
            {
                add_figure(statepainter);
            }
            else if(statepainter == CubicBe)
            {
                add_figure(statepainter);
            }
            this->update();
        }
        else if(curstate == 2)
        {
            curstate = 0;
        }
        else if(curstate == 3)
        {
            curstate = 0;
        }
        else if(curstate == 7)
        {
            curstate = 0;
        }
        else if(curstate == 9)
        {
            curstate = 0;
            remove_figure();
            select_clipper();
            this->update();
        }
    }
}

void MainWindow::on_action_DrawLine_triggered()
{
    oldfigures = figures;
    statepainter = Line;
    curstate = 6;
    this->update();
}

void MainWindow::on_action_DrawCircle_triggered()
{
    oldfigures = figures;
    statepainter = Circle;
    curstate = 6;
    this->update();
}

void MainWindow::on_action_DrawEllipse_triggered()
{
    oldfigures = figures;
    statepainter = Ellipse;
    curstate = 6;
    this->update();
}

void MainWindow::on_action_DrawRectangle_triggered()
{
    oldfigures = figures;
    statepainter = Rectangle;
    curstate = 6;
    this->update();
}

void MainWindow::on_action_Cut_triggered()
{
    oldfigures = figures;
    curstate = 8;
    this->update();
}

void MainWindow::on_actionPolygon_triggered()
{
    oldfigures = figures;
    statepainter = Polygon;
    curstate = 6;
    // 预添加一个多边形图形到表中
    Figures polygon;
    polygon.state = Polygon;
    polygon.content.polygon.count = 0;
    polygon.content.polygon.x[0] = 0;
    polygon.content.polygon.y[0] = 0;
    figures.append(polygon);
    this->update();
}

void MainWindow::on_action_Quadratic_B_zier_curves_triggered()
{
    oldfigures = figures;
    statepainter = QuadBe;
    curstate = 6;
    // 预添加一个二次贝塞尔曲线到表中
    Figures quadbe;
    quadbe.state = QuadBe;
    quadbe.content.quadbe.x[0] = 0;
    quadbe.content.quadbe.y[0] = 0;
    quadbe.content.quadbe.count = 0;
    figures.append(quadbe);
    this->update();
}

void MainWindow::on_action_Cubic_B_zier_curves_triggered()
{
    oldfigures = figures;
    statepainter = CubicBe;
    curstate = 6;
    // 预添加一个三次贝塞尔曲线到表中
    Figures cubicbe;
    cubicbe.state = CubicBe;
    cubicbe.content.cubicbe.x[0] = 0;
    cubicbe.content.cubicbe.y[0] = 0;
    cubicbe.content.cubicbe.count = 0;
    figures.append(cubicbe);
    this->update();
}

void MainWindow::on_action_Undo_triggered()
{
    figures = oldfigures;
    this->update();
}

void MainWindow::on_action_remove_triggered()
{
    oldfigures = figures;
    curstate = 4;
    statepainter = Free;
    statechange = Free;
    //inremoving = true;
}

void MainWindow::on_actionColor_triggered()
{
    oldfigures = figures;
    curstate = 5;
    statepainter = Free;
    statechange = Free;
}

void MainWindow::on_actionChange_Color_triggered()
{
    curcolor = QColorDialog::getColor(Qt::red, this,
                                          tr("Change the color"),
                                          QColorDialog::ShowAlphaChannel);
    curstate = 5;
}

// 绘制图形集
void MainWindow::draw_figures()
{
    QVector<Figures>::iterator iter;
    for (iter=figures.begin(); iter != figures.end(); iter++) {
        if(iter->state == Line)
        {
            /*mainpainter->begin(this);
            mainpainter->drawLine(iter->content.line.x1,
                                  iter->content.line.y1,
                                  iter->content.line.x2,
                                  iter->content.line.y2);
            mainpainter->end();*/
            fillfigure = *iter;
            paint_line(iter->content.line.x1, iter->content.line.y1, iter->content.line.x2, iter->content.line.y2, iter->choose);
        }
        else if(iter->state == Circle)
        {
            //mainpainter->drawEllipse(iter->content.circle.cx, iter->content.circle.cy, iter->content.circle.r, iter->content.circle.r);
            fillfigure = *iter;
            paint_circle(iter->content.circle.cx, iter->content.circle.cy, iter->content.circle.r, iter->choose, iter->color, iter->thiscolor);
        }
        else if(iter->state == Ellipse)
        {
            /*mainpainter->begin(this);
            mainpainter->drawEllipse(iter->content.ellipse.ex,
                                     iter->content.ellipse.ey,
                                     iter->content.ellipse.rx,
                                     iter->content.ellipse.ry);
            mainpainter->end();*/
            fillfigure = *iter;
            paint_ellipse(iter->content.ellipse.ex,
                          iter->content.ellipse.ey,
                          iter->content.ellipse.rx,
                          iter->content.ellipse.ry,
                          iter->choose,
                          iter->color,
                          iter->thiscolor);
        }
        else if(iter->state == Rectangle)
        {
            /*mainpainter->begin(this);
            mainpainter->drawRect(iter->content.rectangle.x,
                                  iter->content.rectangle.y,
                                  iter->content.rectangle.w,
                                  iter->content.rectangle.h);
            mainpainter->end();*/
            fillfigure = *iter;
            paint_rectangle(iter->content.rectangle.x,
                            iter->content.rectangle.y,
                            iter->content.rectangle.x + iter->content.rectangle.w,
                            iter->content.rectangle.y + iter->content.rectangle.h,
                            iter->choose,
                            iter->color,
                            iter->thiscolor);
        }
        else if(iter->state == Polygon)
        {
            fillfigure = *iter;
            paint_polygon(iter->content.polygon.x, iter->content.polygon.y, iter->content.polygon.count,
                          iter->choose, iter->color, iter->thiscolor);
        }
        else if(iter->state == QuadBe)
        {
            if(iter->content.quadbe.count < 3)
                continue;
            mainpainter->begin(this);
            double t = 0.0;

            //double delta = 0.5 / (((iter->content.quadbe.x[2]-iter->content.quadbe.x[0]) > (iter->content.quadbe.y[2]-iter->content.quadbe.y[0]))?
            //            (iter->content.quadbe.x[2] - iter->content.quadbe.x[0]):(iter->content.quadbe.y[2]-iter->content.quadbe.y[0]));

            for(; t <= 1.0; t += 0.0004)
            {
                int px0 = iter->content.quadbe.x[0];
                int px1 = iter->content.quadbe.x[1];
                int px2 = iter->content.quadbe.x[2];
                int py0 = iter->content.quadbe.y[0];
                int py1 = iter->content.quadbe.y[1];
                int py2 = iter->content.quadbe.y[2];
                int nx = pow((1 - t), 2) * px0 + 2 * (1 - t) * t * px1 + pow(t, 2) * px2 + 0.5;
                int ny = pow((1 - t), 2) * py0 + 2 * (1 - t) * t * py1 + pow(t, 2) * py2 + 0.5;
                mainpainter->drawPoint(nx, ny);
            }

            if(iter->choose)
            {
                for(int i = 0; i < 3; i++)
                {
                    mainpainter->drawEllipse(iter->content.quadbe.x[i]-2, iter->content.quadbe.y[i]-2, 4, 4);
                }
            }

            mainpainter->end();
        }
        else if(iter->state == CubicBe)
        {
            if(iter->content.cubicbe.count < 4)
                continue;
            mainpainter->begin(this);
            double t = 0.0;

            for(; t <= 1.0; t += 0.0004)
            {
                int px0 = iter->content.cubicbe.x[0];
                int px1 = iter->content.cubicbe.x[1];
                int px2 = iter->content.cubicbe.x[2];
                int px3 = iter->content.cubicbe.x[3];
                int py0 = iter->content.cubicbe.y[0];
                int py1 = iter->content.cubicbe.y[1];
                int py2 = iter->content.cubicbe.y[2];
                int py3 = iter->content.cubicbe.y[3];
                int nx = pow((1 - t), 3) * px0 + 3 * pow((1 - t), 2) * t * px1 + (1 - t) * pow(t, 2) * px2 + pow(t, 3) * px3 + 0.5;
                int ny = pow((1 - t), 3) * py0 + 3 * pow((1 - t), 2) * t * py1 + (1 - t) * pow(t, 2) * py2 + pow(t, 3) * py3 + 0.5;
                mainpainter->drawPoint(nx, ny);
            }

            if(iter->choose)
            {
                for(int i = 0; i < 4; i++)
                {
                    mainpainter->drawEllipse(iter->content.cubicbe.x[i]-2, iter->content.cubicbe.y[i]-2, 4, 4);
                }
            }

            mainpainter->end();
        }
        else
        {
            continue;
        }
    }
}

void MainWindow::add_figure(int state)
{
    // 不能干扰外界变量
    int regsx = pstartx, regsy = pstarty, regex = pendx, regey = pendy;

    if(state == Line)
    {
        if(pstartx > pendx)
        {
            regsx = pendx;
            regsy = pendy;
            regex = pstartx;
            regey = pstarty;
        }
        Figures line;
        line.state = Line;
        line.content.line.x1 = regsx;
        line.content.line.y1 = regsy;
        line.content.line.x2 = regex;
        line.content.line.y2 = regey;
        figures.append(line);
    }
    else if(state == Circle)
    {
        Figures circle;
        circle.state = Circle;
        circle.content.circle.cx = regsx;
        circle.content.circle.cy = regsy;
        circle.content.circle.r = sqrt(pow(regsx - regex, 2) + pow(regsy - regey, 2));
        figures.append(circle);
    }
    else if(state == Ellipse)
    {
        Figures ellipse;
        ellipse.state = Ellipse;
        ellipse.content.ellipse.ex = (regsx + regex) / 2;
        ellipse.content.ellipse.ey = (regsy + regey) / 2;
        ellipse.content.ellipse.rx = abs(regex - regsx) / 2;
        ellipse.content.ellipse.ry = abs(regey - regsy) / 2;
        figures.append(ellipse);
    }
    else if(state == Rectangle)
    {
        if(pstartx < pendx)
        {
            // 左上
            if(pstarty < pendy) ;
            // 左下
            else
            {
                regsy = pendy;
                regey = pstarty;
            }
        }
        else
        {
            // 右上
            if(pstarty < pendy)
            {
                regsx = pendx;
                regex = pstartx;
            }
            // 右下
            else
            {
                regsx = pendx;
                regsy = pendy;
                regex = pstartx;
                regey = pstarty;
            }
        }
        Figures rectangle;
        rectangle.state = Rectangle;
        rectangle.content.rectangle.x = regsx;
        rectangle.content.rectangle.y = regsy;
        rectangle.content.rectangle.h = abs(regey - regsy);
        rectangle.content.rectangle.w = abs(regex - regsx);
        figures.append(rectangle);
    }
    else if(state == Polygon)
    {
        int i = figures.length()-1;
        assert(figures[i].state == Polygon);
        int count = figures[i].content.polygon.count;

        figures[i].content.polygon.x[count] = regex;
        figures[i].content.polygon.y[count] = regey;
        figures[i].content.polygon.count = count + 1;
        assert(count < 200);

        // 判断多边形绘图是否该结束
        //qDebug() << abs(figures[i].content.polygon.x[0]-regex) << abs(figures[i].content.polygon.y[0]-regey);
        if(count > 0 && abs(figures[i].content.polygon.x[0]-regex) < 10 && abs(figures[i].content.polygon.y[0]-regey) < 10)
        {
            figures[i].content.polygon.x[count] = figures[i].content.polygon.x[0];
            figures[i].content.polygon.y[count] = figures[i].content.polygon.y[0];
            statepainter = Free;
            curstate = 0;
            return;
        }
    }
    else if(state == QuadBe)
    {
        int i = figures.length()-1;
        assert(figures[i].state == QuadBe);
        int count = figures[i].content.quadbe.count;
        figures[i].content.quadbe.x[count] = regex;
        figures[i].content.quadbe.y[count] = regey;
        figures[i].content.quadbe.count++;
        count++;
        // 结束曲线的绘制
        if(count >= 3)
        {
            statepainter = Free;
            curstate = 0;
            return;
        }
    }
    else if(state == CubicBe)
    {
        int i = figures.length()-1;
        assert(figures[i].state == CubicBe);
        int count = figures[i].content.cubicbe.count;
        figures[i].content.cubicbe.x[count] = regex;
        figures[i].content.cubicbe.y[count] = regey;
        figures[i].content.cubicbe.count++;
        count++;
        // 结束曲线的绘制
        if(count >= 4)
        {
            statepainter = Free;
            curstate = 0;
            return;
        }
    }
}

void MainWindow::remove_figure()
{
    figures.pop_back();
}

bool MainWindow::choosefigure(int clix, int cliy)
{   
    QVector<Figures>::iterator iter;

    // 还原所有图形到未选中状态
    for (iter = figures.begin(); iter != figures.end(); iter++) iter->choose = false;

    // 高亮选中图形，同一时间仅能选中一个
    for (iter = figures.begin(); iter != figures.end(); iter++)
    {
        if(iter->state == Line)
        {
            // 判断是否点到该图形
            double A = (double)1 / (iter->content.line.x2 - iter->content.line.x1);
            double B = (double)1 / (iter->content.line.y1 - iter->content.line.y2);
            double C = (iter->content.line.y1 * (-B)) - (iter->content.line.x1 * A);
            double d = abs(A * clix + B * cliy + C) / sqrt(A * A + B * B);

            if(d < 5 && d > -5 && clix >= iter->content.line.x1 && clix <= iter->content.line.x2)
            {
                qDebug() << "Click line.";
                iter->choose = true;
                this->update();
                return true;
            }
        }
        else if (iter->state == Circle)
        {
            int d = sqrt(pow(clix - iter->content.circle.cx, 2) + pow(cliy - iter->content.circle.cy, 2));
            if(abs(d - iter->content.circle.r) < 5)
            {
                qDebug() << "Click circle.";
                iter->choose = true;
                this->update();
                return true;
            }
        }
        else if (iter->state == Ellipse)
        {
            if(iter->content.ellipse.rx >= iter->content.ellipse.ry)
            {
                // PF1 + PF2 = 2a
                int xf1 = iter->content.ellipse.ex - sqrt(pow(iter->content.ellipse.rx, 2) - pow(iter->content.ellipse.ry, 2));
                int xf2 = iter->content.ellipse.ex + sqrt(pow(iter->content.ellipse.rx, 2) - pow(iter->content.ellipse.ry, 2));
                int d = sqrt(pow(clix - xf1, 2) + pow(cliy - iter->content.ellipse.ey, 2)) + sqrt(pow(clix - xf2, 2) + pow(cliy - iter->content.ellipse.ey, 2));
                if(d - 2 * iter->content.ellipse.rx < 5 && d - 2 * iter->content.ellipse.rx > -5)
                {
                    qDebug() << "Click ellipse.";
                    iter->choose = true;
                    this->update();
                    return true;
                }
            }
            else
            {
                // PF1 + PF2 = 2a
                int yf1 = iter->content.ellipse.ey - sqrt(pow(iter->content.ellipse.ry, 2) - pow(iter->content.ellipse.rx, 2));
                int yf2 = iter->content.ellipse.ey + sqrt(pow(iter->content.ellipse.ry, 2) - pow(iter->content.ellipse.rx, 2));
                int d = sqrt(pow(clix - iter->content.ellipse.ex, 2) + pow(cliy - yf1, 2)) + sqrt(pow(clix - iter->content.ellipse.ex, 2) + pow(cliy - yf2, 2));
                if(d - 2 * iter->content.ellipse.ry < 5 && d - 2 * iter->content.ellipse.ry > -5)
                {
                    qDebug() << "Click ellipse.";
                    iter->choose = true;
                    this->update();
                    return true;
                }
            }
        }
        else if (iter->state == Rectangle)
        {
            int ld, rd, ud, bd;
            ld = abs(clix - iter->content.rectangle.x);
            rd = abs(clix - iter->content.rectangle.x - iter->content.rectangle.w);
            ud = abs(cliy - iter->content.rectangle.y);
            bd = abs(cliy - iter->content.rectangle.y - iter->content.rectangle.h);
            if( ( (ld < 5 || rd < 5) && cliy >= iter->content.rectangle.y && cliy <= iter->content.rectangle.y + iter->content.rectangle.h)
                || ( (ud < 5 || bd < 5) && clix >= iter->content.rectangle.x && cliy <= iter->content.rectangle.x + iter->content.rectangle.w))
            {
                qDebug() << "Click rectangle.";
                iter->choose = true;
                this->update();
                return true;
            }
        }
        else if (iter->state == Polygon)
        {
            int count = iter->content.polygon.count;
            for(int i = 0; i < count-1; i++)
            {
                int x1 = iter->content.polygon.x[i];
                int y1 = iter->content.polygon.y[i];
                int x2 = iter->content.polygon.x[i+1];
                int y2 = iter->content.polygon.y[i+1];
                double A = (double)1 / (x2 - x1);
                double B = (double)1 / (y1 - y2);
                double C = (y1 * (-B)) - (x1 * A);
                double d = abs(A * clix + B * cliy + C) / sqrt(A * A + B * B);

                int pminx = (iter->content.polygon.x[i] < iter->content.polygon.x[i+1])? iter->content.polygon.x[i]:iter->content.polygon.x[i+1];
                int pmaxx = (iter->content.polygon.x[i] > iter->content.polygon.x[i+1])? iter->content.polygon.x[i]:iter->content.polygon.x[i+1];

                if(d < 5 && d > -5 && clix >= pminx && clix <= pmaxx)
                {
                    qDebug() << "Click Polygon " << i;
                    iter->choose = true;
                    this->update();
                    return true;
                }
            }
        }
        else if (iter->state == QuadBe)
        {
            double t = 0.0;

            //double delta = 0.5 / (((iter->content.quadbe.x[2]-iter->content.quadbe.x[0]) > (iter->content.quadbe.y[2]-iter->content.quadbe.y[0]))?
            //            (iter->content.quadbe.x[2] - iter->content.quadbe.x[0]):(iter->content.quadbe.y[2]-iter->content.quadbe.y[0]));

            for(; t <= 1.0; t += 0.0004)
            {
                int px0 = iter->content.quadbe.x[0];
                int px1 = iter->content.quadbe.x[1];
                int px2 = iter->content.quadbe.x[2];
                int py0 = iter->content.quadbe.y[0];
                int py1 = iter->content.quadbe.y[1];
                int py2 = iter->content.quadbe.y[2];
                int nx = pow((1 - t), 2) * px0 + 2 * (1 - t) * t * px1 + pow(t, 2) * px2 + 0.5;
                int ny = pow((1 - t), 2) * py0 + 2 * (1 - t) * t * py1 + pow(t, 2) * py2 + 0.5;

                if(abs(clix-nx) < 5 && abs(cliy-ny) < 5)
                {
                    qDebug() << "Click QuadBezier Curve.";
                    iter->choose = true;
                    this->update();
                    return true;
                }
            }
        }
        else if (iter->state == CubicBe)
        {
            double t = 0.0;

            for(; t <= 1.0; t += 0.0004)
            {
                int px0 = iter->content.cubicbe.x[0];
                int px1 = iter->content.cubicbe.x[1];
                int px2 = iter->content.cubicbe.x[2];
                int px3 = iter->content.cubicbe.x[3];
                int py0 = iter->content.cubicbe.y[0];
                int py1 = iter->content.cubicbe.y[1];
                int py2 = iter->content.cubicbe.y[2];
                int py3 = iter->content.cubicbe.y[3];
                int nx = pow((1 - t), 3) * px0 + 3 * pow((1 - t), 2) * t * px1 + (1 - t) * pow(t, 2) * px2 + pow(t, 3) * px3 + 0.5;
                int ny = pow((1 - t), 3) * py0 + 3 * pow((1 - t), 2) * t * py1 + (1 - t) * pow(t, 2) * py2 + pow(t, 3) * py3 + 0.5;

                if(abs(clix-nx) < 5 && abs(cliy-ny) < 5)
                {
                    qDebug() << "Click CubicBezier Curve.";
                    iter->choose = true;
                    this->update();
                    return true;
                }
            }
        }
    }
    this->update();
    return false;
}

bool MainWindow::removefigure(int clix, int cliy)
{
    QVector<Figures>::iterator iter;

    // 删除选中图形
    for (iter = figures.begin(); iter != figures.end(); iter++)
    {
        if(iter->state == Line)
        {
            // 判断鼠标是否点击到该图形
            double A = (double)1 / (iter->content.line.x2 - iter->content.line.x1);
            double B = (double)1 / (iter->content.line.y1 - iter->content.line.y2);
            double C = (iter->content.line.y1 * (-B)) - (iter->content.line.x1 * A);
            double d = abs(A * clix + B * cliy + C) / sqrt(A * A + B * B);

            if(d < 5 && d > -5 && clix >= iter->content.line.x1 && clix <= iter->content.line.x2)
            {
                qDebug() << "Remove Line.";
                curstate = 0;
                figures.erase(iter);
                this->update();
                return true;
            }
        }
        else if (iter->state == Circle)
        {
            int d = sqrt(pow(clix - iter->content.circle.cx, 2) + pow(cliy - iter->content.circle.cy, 2));
            if(abs(d - iter->content.circle.r) < 5)
            {
                qDebug() << "Remove Circle.";
                curstate = 0;
                figures.erase(iter);
                this->update();
                return true;
            }
        }
        else if (iter->state == Ellipse)
        {
            // PF1 + PF2 = 2a
            if(iter->content.ellipse.rx >= iter->content.ellipse.ry)
            {
                int xf1 = iter->content.ellipse.ex - sqrt(pow(iter->content.ellipse.rx, 2) - pow(iter->content.ellipse.ry, 2));
                int xf2 = iter->content.ellipse.ex + sqrt(pow(iter->content.ellipse.rx, 2) - pow(iter->content.ellipse.ry, 2));
                int d = sqrt(pow(clix - xf1, 2) + pow(cliy - iter->content.ellipse.ey, 2)) + sqrt(pow(clix - xf2, 2) + pow(cliy - iter->content.ellipse.ey, 2));
                if(d - 2 * iter->content.ellipse.rx < 5 && d - 2 * iter->content.ellipse.rx > -5)
                {
                    qDebug() << "Remove Ellipse.";
                    curstate = 0;
                    figures.erase(iter);
                    this->update();
                    return true;
                }
            }
            else
            {
                // PF1 + PF2 = 2a
                int yf1 = iter->content.ellipse.ey - sqrt(pow(iter->content.ellipse.ry, 2) - pow(iter->content.ellipse.rx, 2));
                int yf2 = iter->content.ellipse.ey + sqrt(pow(iter->content.ellipse.ry, 2) - pow(iter->content.ellipse.rx, 2));
                int d = sqrt(pow(clix - iter->content.ellipse.ex, 2) + pow(cliy - yf1, 2)) + sqrt(pow(clix - iter->content.ellipse.ex, 2) + pow(cliy - yf2, 2));
                if(d - 2 * iter->content.ellipse.ry < 5 && d - 2 * iter->content.ellipse.ry > -5)
                {
                    qDebug() << "Remove Ellipse.";
                    curstate = 0;
                    figures.erase(iter);
                    this->update();
                    return true;
                }
            }
        }
        else if (iter->state == Rectangle)
        {
            int ld, rd, ud, bd;
            ld = abs(clix - iter->content.rectangle.x);
            rd = abs(clix - iter->content.rectangle.x - iter->content.rectangle.w);
            ud = abs(cliy - iter->content.rectangle.y);
            bd = abs(cliy - iter->content.rectangle.y - iter->content.rectangle.h);
            if( ( (ld < 5 || rd < 5) && cliy >= iter->content.rectangle.y && cliy <= iter->content.rectangle.y + iter->content.rectangle.h)
                || ( (ud < 5 || bd < 5) && clix >= iter->content.rectangle.x && cliy <= iter->content.rectangle.x + iter->content.rectangle.w))
            {
                qDebug() << "Remove Rectangle.";
                curstate = 0;
                figures.erase(iter);
                this->update();
                return true;
            }
        }
        else if (iter->state == Polygon)
        {
            int count = iter->content.polygon.count;
            for(int i = 0; i < count-1; i++)
            {
                int x1 = iter->content.polygon.x[i];
                int y1 = iter->content.polygon.y[i];
                int x2 = iter->content.polygon.x[i+1];
                int y2 = iter->content.polygon.y[i+1];
                double A = (double)1 / (x2 - x1);
                double B = (double)1 / (y1 - y2);
                double C = (y1 * (-B)) - (x1 * A);
                double d = abs(A * clix + B * cliy + C) / sqrt(A * A + B * B);

                int pminx = (iter->content.polygon.x[i] < iter->content.polygon.x[i+1])? iter->content.polygon.x[i]:iter->content.polygon.x[i+1];
                int pmaxx = (iter->content.polygon.x[i] > iter->content.polygon.x[i+1])? iter->content.polygon.x[i]:iter->content.polygon.x[i+1];

                if(d < 5 && d > -5 && clix >= pminx && clix <= pmaxx)
                {
                    qDebug() << "Remove Polygon.";
                    curstate = 0;
                    figures.erase(iter);
                    this->update();
                    return true;
                }
            }
        }
        else if (iter->state == QuadBe)
        {
            double t = 0.0;

            for(; t <= 1.0; t += 0.0004)
            {
                int px0 = iter->content.quadbe.x[0];
                int px1 = iter->content.quadbe.x[1];
                int px2 = iter->content.quadbe.x[2];
                int py0 = iter->content.quadbe.y[0];
                int py1 = iter->content.quadbe.y[1];
                int py2 = iter->content.quadbe.y[2];
                int nx = pow((1 - t), 2) * px0 + 2 * (1 - t) * t * px1 + pow(t, 2) * px2 + 0.5;
                int ny = pow((1 - t), 2) * py0 + 2 * (1 - t) * t * py1 + pow(t, 2) * py2 + 0.5;

                if(abs(clix-nx) < 5 && abs(cliy-ny) < 5)
                {
                    qDebug() << "Remove QuadBezier Curve.";
                    curstate = 0;
                    figures.erase(iter);
                    this->update();
                    return true;
                }
            }
        }
        else if (iter->state == CubicBe)
        {
            double t = 0.0;

            for(; t <= 1.0; t += 0.0004)
            {
                int px0 = iter->content.cubicbe.x[0];
                int px1 = iter->content.cubicbe.x[1];
                int px2 = iter->content.cubicbe.x[2];
                int px3 = iter->content.cubicbe.x[3];
                int py0 = iter->content.cubicbe.y[0];
                int py1 = iter->content.cubicbe.y[1];
                int py2 = iter->content.cubicbe.y[2];
                int py3 = iter->content.cubicbe.y[3];
                int nx = pow((1 - t), 3) * px0 + 3 * pow((1 - t), 2) * t * px1 + (1 - t) * pow(t, 2) * px2 + pow(t, 3) * px3 + 0.5;
                int ny = pow((1 - t), 3) * py0 + 3 * pow((1 - t), 2) * t * py1 + (1 - t) * pow(t, 2) * py2 + pow(t, 3) * py3 + 0.5;

                if(abs(clix-nx) < 5 && abs(cliy-ny) < 5)
                {
                    qDebug() << "Remove CubicBezier Curve.";
                    curstate = 0;
                    figures.erase(iter);
                    this->update();
                    return true;
                }
            }
        }
    }
    this->update();
    return false;
}

int MainWindow::changefigure(int clix, int cliy)
{
    int rtvalue = 0;
    int count = 0;
    QVector<Figures>::iterator iter;
    for (iter = figures.begin(); iter != figures.end(); iter++, count++)
    {
        if(iter->state == Line)
        {
            // 判断是否点到调整点
            if( ( abs(clix - iter->content.line.x1) < 5 && abs(cliy - iter->content.line.y1) < 5 )
                || ( abs(clix - iter->content.line.x2) < 5 && abs(cliy - iter->content.line.y2) < 5 ) )
            {
                statechange = iter->state;

                // 设置pstartx和pstarty
                if(abs(clix - iter->content.line.x1) < 5 && abs(cliy - iter->content.line.y1) < 5)
                {
                    pstartx = iter->content.line.x2;
                    pstarty = iter->content.line.y2;
                }
                else
                {
                    pstartx = iter->content.line.x1;
                    pstarty = iter->content.line.y1;
                }

                // 设置参数
                rtvalue = 1;
                change_no = count;
                break;
            }
            else if(abs(clix - (iter->content.line.x1 + iter->content.line.x2)/2) < 5 && abs(cliy - (iter->content.line.y1 + iter->content.line.y2)/2) < 5)
            {
                statechange = iter->state;

                // 设置mstartx和mstarty
                mstartx = clix;
                mstarty = cliy;

                // 设置参数
                rtvalue = 2;
                change_no = count;
                break;
            }
            else if(abs(clix - (iter->content.line.x1 + iter->content.line.x2)/2) < 5 && abs(cliy - (iter->content.line.y1 + iter->content.line.y2)/2 + 30) < 5)
            {
                qDebug() << "Rotate the Line.";
                statechange = iter->state;

                mstartx = (iter->content.line.x1 + iter->content.line.x2)/2 + 0.5;
                mstarty = (iter->content.line.y1 + iter->content.line.y2)/2 + 0.5;

                // 设置参数
                rtvalue = 3;
                change_no = count;
                break;
            }
        }
        else if(iter->state == Circle)
        {
            // 鼠标点击变换点
            if(abs(clix - iter->content.circle.cx) < 5 && abs(cliy - iter->content.circle.cy + iter->content.circle.r) < 5)
            {
                statechange = iter->state;

                // 设置变换初值
                mstartx = clix;
                mstarty = cliy;

                rtvalue = 1;
                change_no = count;
                break;
            }
            // 鼠标点击平移点
            else if(abs(clix - iter->content.circle.cx) < 5 && abs(cliy - iter->content.circle.cy) < 5)
            {
                statechange = iter->state;

                // 设置平移初值
                mstartx = clix;
                mstarty = cliy;

                rtvalue = 2;
                change_no = count;
                break;
            }
        }
        else if(iter->state == Ellipse)
        {
            if( (abs(clix - iter->content.ellipse.ex + iter->content.ellipse.rx) < 5 && abs(cliy - iter->content.ellipse.ey + iter->content.ellipse.ry) < 5)
                    ||  (abs(clix - iter->content.ellipse.ex - iter->content.ellipse.rx) < 5 && abs(cliy - iter->content.ellipse.ey - iter->content.ellipse.ry) < 5) )
            {
                statechange = iter->state;

                if(abs(clix - iter->content.ellipse.ex + iter->content.ellipse.rx) < 5 && abs(cliy - iter->content.ellipse.ey + iter->content.ellipse.ry) < 5)
                {
                    pstartx = iter->content.ellipse.ex + iter->content.ellipse.rx;
                    pstarty = iter->content.ellipse.ey + iter->content.ellipse.ry;
                }
                else
                {
                    pstartx = iter->content.ellipse.ex - iter->content.ellipse.rx;
                    pstarty = iter->content.ellipse.ey - iter->content.ellipse.ry;
                }

                rtvalue = 1;
                change_no = count;
                break;
            }
            else if(abs(clix - iter->content.ellipse.ex) < 5 && abs(cliy - iter->content.ellipse.ey) < 5)
            {
                 statechange = iter->state;

                 mstartx = clix;
                 mstarty = cliy;

                 rtvalue = 2;
                 change_no = count;
                 break;
            }
            else if(abs(clix - iter->content.ellipse.ex) < 5 && abs(cliy - iter->content.ellipse.ey + 30) < 5)
            {
                statechange = iter->state;

                mstartx = iter->content.ellipse.ex;
                mstarty = iter->content.ellipse.ey;

                rtvalue = 3;
                rotate_ellipse_rectangle = false;
                change_no = count;
                break;
            }
        }
        else if(iter->state == Rectangle)
        {
            if( (abs(clix - iter->content.rectangle.x) < 5 && abs(cliy - iter->content.rectangle.y) < 5)
                    ||  (abs(clix - iter->content.rectangle.x - iter->content.rectangle.w) < 5 && abs(cliy - iter->content.rectangle.y - iter->content.rectangle.h) < 5) )
            {
                statechange = iter->state;

                if(abs(clix - iter->content.rectangle.x) < 5 && abs(cliy - iter->content.rectangle.y) < 5)
                {
                    pstartx = iter->content.rectangle.x + iter->content.rectangle.w;
                    pstarty = iter->content.rectangle.y + iter->content.rectangle.h;
                }
                else
                {
                    pstartx = iter->content.rectangle.x;
                    pstarty = iter->content.rectangle.y;
                }

                rtvalue = 1;
                change_no = count;
                break;
            }
            else if(abs(clix - iter->content.rectangle.x - iter->content.rectangle.w/2) < 5
                    && abs(cliy - iter->content.rectangle.y - iter->content.rectangle.h/2) < 5)
            {
                statechange = iter->state;

                mstartx = clix;
                mstarty = cliy;

                rtvalue = 2;
                change_no = count;
                break;
            }
            else if(abs(clix - iter->content.rectangle.x - iter->content.rectangle.w/2) < 5
                    && abs(cliy - iter->content.rectangle.y - iter->content.rectangle.h/2 + 30) < 5)
            {
                statechange = iter->state;

                mstartx = iter->content.rectangle.x + iter->content.rectangle.w/2 + 0.5;
                mstarty = iter->content.rectangle.y + iter->content.rectangle.h/2 + 0.5;

                rtvalue = 3;
                rotate_ellipse_rectangle = false;
                change_no = count;
                break;
            }
        }
        else if(iter->state == Polygon)
        {
            for(int i = 0; i < iter->content.polygon.count; i++)
            {
                if(abs(clix - iter->content.polygon.x[i]) < 5 && abs(cliy - iter->content.polygon.y[i]) < 5)
                {
                    qDebug() << "Change the Polygon. " << i << endl;
                    statechange = iter->state;
                    change_no = count;
                    polygon_point_no = i;
                    rtvalue = 1;
                    return rtvalue;
                }
            }

            int max_x = 0, max_y = 0, min_x = 10000, min_y = 10000;
            for(int i = 0; i < iter->content.polygon.count; i++)
            {
                max_x = (max_x > iter->content.polygon.x[i])?max_x:iter->content.polygon.x[i];
                min_x = (min_x < iter->content.polygon.x[i])?min_x:iter->content.polygon.x[i];
                max_y = (max_y > iter->content.polygon.y[i])?max_y:iter->content.polygon.y[i];
                min_y = (min_y < iter->content.polygon.y[i])?min_y:iter->content.polygon.y[i];
            }
            if(abs(clix - (max_x+min_x)/2) < 5 && abs(cliy - (max_y+min_y)/2) < 5)
            {
                qDebug() << "Move the Polygon.";
                statechange = iter->state;
                change_no = count;

                mstartx = clix;
                mstarty = cliy;

                rtvalue = 2;
                return rtvalue;
            }
            else if(abs(clix - (max_x+min_x)/2) < 5 && abs(cliy - (max_y+min_y)/2 + 30) < 5)
            {
                qDebug() << "Rotate the Polygon.";
                statechange = iter->state;
                change_no = count;

                mstartx = (max_x+min_x)/2 + 0.5;
                mstarty = (max_y+min_y)/2 + 0.5;

                rtvalue = 3;
                return rtvalue;
            }
        }
        else if(iter->state == QuadBe)
        {
            for(int i = 0; i < 3; i++)
            {
                if(abs(clix - iter->content.quadbe.x[i]) < 5 && abs(cliy - iter->content.quadbe.y[i]) < 5)
                {
                    qDebug() << "Change the QuadBezier Curve.";
                    statechange = iter->state;
                    change_no = count;
                    polygon_point_no = i;
                    rtvalue = 1;
                    return rtvalue;
                }
            }
        }
        else if(iter->state == CubicBe)
        {
            for(int i = 0; i < 4; i++)
            {
                if(abs(clix - iter->content.cubicbe.x[i]) < 5 && abs(cliy - iter->content.cubicbe.y[i]) < 5)
                {
                    qDebug() << "Change the CubicBezier Curve.";
                    statechange = iter->state;
                    change_no = count;
                    polygon_point_no = i;
                    rtvalue = 1;
                    return rtvalue;
                }
            }
        }
    }
    return rtvalue;
}

int MainWindow::colorfigure(int clix, int cliy) {
    int count = 0;
    QVector<Figures>::iterator iter;
    for (iter = figures.begin(); iter != figures.end(); iter++, count++)
    {
        if(iter->state == Circle)
        {
            int r = iter->content.circle.r;
            int cx = iter->content.circle.cx;
            int cy = iter->content.circle.cy;
            if(pow(clix - cx, 2) + pow(cliy - cy, 2) <= pow(r, 2))
            {
                // 对这个圆染色
                iter->color = true;
                iter->thiscolor = curcolor;
                break;
            }
        }
        else if(iter->state == Ellipse)
        {
            if(iter->content.ellipse.rx >= iter->content.ellipse.ry)
            {
                int xf1 = iter->content.ellipse.ex - sqrt(pow(iter->content.ellipse.rx, 2) - pow(iter->content.ellipse.ry, 2));
                int xf2 = iter->content.ellipse.ex + sqrt(pow(iter->content.ellipse.rx, 2) - pow(iter->content.ellipse.ry, 2));
                int d = sqrt(pow(clix - xf1, 2) + pow(cliy - iter->content.ellipse.ey, 2)) + sqrt(pow(clix - xf2, 2) + pow(cliy - iter->content.ellipse.ey, 2));
                if(d < 2 * iter->content.ellipse.rx)
                {
                    iter->color = true;
                    iter->thiscolor = curcolor;
                    break;
                }
            }
            else
            {
                int yf1 = iter->content.ellipse.ey - sqrt(pow(iter->content.ellipse.ry, 2) - pow(iter->content.ellipse.rx, 2));
                int yf2 = iter->content.ellipse.ey + sqrt(pow(iter->content.ellipse.ry, 2) - pow(iter->content.ellipse.rx, 2));
                int d = sqrt(pow(clix - iter->content.ellipse.ex, 2) + pow(cliy - yf1, 2)) + sqrt(pow(clix - iter->content.ellipse.ex, 2) + pow(cliy - yf2, 2));
                if(d < 2 * iter->content.ellipse.ry)
                {
                    iter->color = true;
                    iter->thiscolor = curcolor;
                    break;
                }
            }
        }
        else if(iter->state == Rectangle)
        {
            if(clix > iter->content.rectangle.x && clix < iter->content.rectangle.x + iter->content.rectangle.w
                    && cliy > iter->content.rectangle.y && cliy < iter->content.rectangle.y + iter->content.rectangle.h)
            {
                iter->color = true;
                iter->thiscolor = curcolor;
                break;
            }
        }
        else if(iter->state == Polygon)
        {
            if(point_in_figure(figures[count], clix, cliy) == true)
            {
                iter->color = true;
                iter->thiscolor = curcolor;
                break;
            }
        }
    }
    return count;
}

// 区域染色:边界填充算法
void MainWindow::fillcolor(Figures f, QColor c, QPoint p0)
{
    // p0仅在非多边形染色时作为种子点使用

    QPainter painter(this);
    //定义画笔
    QPen pen(c);
    painter.setPen(pen);
    if(f.state != Polygon)
    {
        fillleft(f, p0);
        int tcount1 = fill_point_count;
        for(int i = 0; i < tcount1; i++)
        {
            fillup(f, fill_point_list[i]);
            filldown(f, fill_point_list[i]);
        }

        int tcount2 = fill_point_count;
        fillright(f, p0);
        int tcount3 = fill_point_count;
        for(int i = tcount2; i < tcount3; i++)
        {
            fillup(f, fill_point_list[i]);
            filldown(f, fill_point_list[i]);
        }

        painter.drawPoints(fill_point_list, fill_point_count);
    }
    else
    {
        intersect_point_count = 0;
        int min_y = 100000, max_y = 0;
        for(int i = 0; i < f.content.polygon.count; i++)
        {
            min_y = (min_y < f.content.polygon.y[i])? min_y:f.content.polygon.y[i];
            max_y = (max_y > f.content.polygon.y[i])? max_y:f.content.polygon.y[i];
        }
        // max_y - min_y = 扫描线的条数
        for(int y0 = min_y; y0 <= max_y; y0++)
        {
            // 对count个点决定的count-1条直线，计算交点
            int alreadyx[1000]; int alreadycount = 0;
            for(int i = 0; i < f.content.polygon.count-1; i++)
            {
                int x1 = f.content.polygon.x[i];
                int x2 = f.content.polygon.x[i+1];
                int y1 = f.content.polygon.y[i];
                int y2 = f.content.polygon.y[i+1];
                if(y1 > y2)
                {
                    x1 = f.content.polygon.x[i+1];
                    x2 = f.content.polygon.x[i];
                    y1 = f.content.polygon.y[i+1];
                    y2 = f.content.polygon.y[i];
                }
                // 不满足条件，舍弃这条直线
                if(y1 > y0 || y2 < y0) continue;

                int x0 = double((y0 - y1) * (x2 - x1))/(y2 - y1) + x1;

                // 以防万一设置的跳过条件
                if(x0 < 0) continue;

                alreadyx[alreadycount++] = x0;
                //qDebug() << "Line" << i << QPoint(x0, y0);
            }
            // TODO:如果这个多边形是部分隐藏在y < 0 区域内的话，添加一个(1, y0)的起始点
            /*if(point_in_figure(f, 1, y0) == true)
            {
                qDebug() << "Line" << y0 << "in figure.";
                alreadyx[alreadycount++] = 1;
            }*/

            // 对alreadyx中的数据进行排序再统一放入
            std::sort(alreadyx, alreadyx+alreadycount);

            for(int i = 0; i < alreadycount; i++)
            {
                intersect_point_list[intersect_point_count++] = QPoint(alreadyx[i], y0);
            }
        }

        // 根据交点填充多边形
        // 当fillflag为true时，开始填充
        bool fillflag = false;
        int scanliney = -1;
        for(int i = 0; i < intersect_point_count; i++)
        {
            if(intersect_point_list[i].y() != scanliney)
            {
                fillflag = true;
                scanliney = intersect_point_list[i].y();
            }
            else
            {
                // 遇到同一条扫描线上的一个新点，切换状态
                fillflag = !fillflag;
            }
            if(fillflag)
            {
                // 如果两个点相等的话，这是两条线段的拐点，需要做一些处理动作
                // 如果该点与之后的一个点相等，只考虑后一个点即可
                if(intersect_point_list[i].x() == intersect_point_list[i+1].x())
                {
                    //qDebug() << "EQUAL!" << intersect_point_list[i] << intersect_point_list[i+1];
                    //后一个点后在多边形内的话，只画后一个点；否则就不画（继续做下去相当不画，但是保证了fillflag的一致性）
                    if(point_in_figure(f, intersect_point_list[i].x() + 1, intersect_point_list[i].y()))
                        i++;
                }
                // 如果该点与前一个点相等，如果该点之后不在图形内，跳过这个点并且将fillflag置为false
                else if(i > 0 && intersect_point_list[i].x() == intersect_point_list[i-1].x() && intersect_point_list[i].y() == intersect_point_list[i-1].y())
                {
                    if(!point_in_figure(f, intersect_point_list[i].x() + 1, intersect_point_list[i].y()))
                    {
                        fillflag = false;
                        continue;
                    }
                }

                // 最后一个点不用考虑
                if(i+1 >= intersect_point_count) continue;

                // 到下一条扫描线了
                if(intersect_point_list[i+1].y() != scanliney) continue;

                // 开始填充点到列表中，填充范围为(x1, y0)到(x2, y0)
                int y0 = intersect_point_list[i].y();
                int x1 = intersect_point_list[i].x();
                int x2 = intersect_point_list[i+1].x();

                // 之前做了排序，因此一定要保证这里的有序性
                assert(x1 <= x2);

                for(int j = x1; j <= x2; j++)
                {
                    fill_point_list[fill_point_count++] = QPoint(j, y0);
                }

                // 跳过下一个点（已处理完毕），置fillflag为false
                i++;
                fillflag = false;
            }
        }
        painter.drawPoints(fill_point_list, fill_point_count);
    }
}

void MainWindow::fillleft(Figures f, QPoint p0)
{
    int x = p0.x(), y = p0.y();
    for(; (!point_in_edge(f, QPoint(x, y))) && x >= 0; x--)
    {
        fill_point_list[fill_point_count++] = QPoint(x, y);
    }
}

void MainWindow::fillup(Figures f, QPoint p0)
{
    int x = p0.x(), y = p0.y();
    for(; (!point_in_edge(f, QPoint(x, y))) && y >= 0; y--)
    {
        fill_point_list[fill_point_count++] = QPoint(x, y);
    }
}

void MainWindow::fillright(Figures f, QPoint p0)
{
    int x = p0.x(), y = p0.y();
    // TODO:
    for(; (!point_in_edge(f, QPoint(x, y))) && x <= this->frameGeometry().width(); x++)
    {
        fill_point_list[fill_point_count++] = QPoint(x, y);
    }
}

void MainWindow::filldown(Figures f, QPoint p0)
{
    int x = p0.x(), y = p0.y();
    // TODO:
    for(; (!point_in_edge(f, QPoint(x, y))) && y <= this->frameGeometry().height(); y++)
    {
        fill_point_list[fill_point_count++] = QPoint(x, y);
    }
}

bool MainWindow::point_in_edge(Figures f, QPoint p)
{
    int clix = p.x();
    int cliy = p.y();
    if (f.state == Circle)
    {
        int d = sqrt(pow(clix - f.content.circle.cx, 2) + pow(cliy - f.content.circle.cy, 2));
        if(abs(d - f.content.circle.r) <= 1)
        {
            return true;
        }
    }
    else if (f.state == Ellipse)
    {
        if(f.content.ellipse.rx >= f.content.ellipse.ry)
        {
            // PF1 + PF2 = 2a
            int xf1 = f.content.ellipse.ex - sqrt(pow(f.content.ellipse.rx, 2) - pow(f.content.ellipse.ry, 2));
            int xf2 = f.content.ellipse.ex + sqrt(pow(f.content.ellipse.rx, 2) - pow(f.content.ellipse.ry, 2));
            int d = sqrt(pow(clix - xf1, 2) + pow(cliy - f.content.ellipse.ey, 2)) + sqrt(pow(clix - xf2, 2) + pow(cliy - f.content.ellipse.ey, 2));
            if(abs(d - 2 * f.content.ellipse.rx) <= 1)
            {
                return true;
            }
        }
        else
        {
            int yf1 = f.content.ellipse.ey - sqrt(pow(f.content.ellipse.ry, 2) - pow(f.content.ellipse.rx, 2));
            int yf2 = f.content.ellipse.ey + sqrt(pow(f.content.ellipse.ry, 2) - pow(f.content.ellipse.rx, 2));
            int d = sqrt(pow(clix - f.content.ellipse.ex, 2) + pow(cliy - yf1, 2)) + sqrt(pow(clix - f.content.ellipse.ex, 2) + pow(cliy - yf2, 2));
            if(abs(d - 2 * f.content.ellipse.ry) <= 1)
            {
                return true;
            }
        }
    }
    return false;
}

// 判断一个点是否在一个图形（多边形）内部
bool MainWindow::point_in_figure(Figures f, int x0, int y0)
{
    if(f.state == Polygon)
    {
        int incount = 0;

        // 算法参考www.cnblogs.com/tuyang1129/p/9390376.html
        int abx, aby;
        abx = this->frameGeometry().width() - x0; aby = 0;

        for(int i = 0; i < f.content.polygon.count-1; i++)
        {
            int acx, acy;
            acx = f.content.polygon.x[i] - x0;
            acy = f.content.polygon.y[i] - y0;

            int adx, ady;
            adx = f.content.polygon.x[i+1] - x0;
            ady = f.content.polygon.y[i+1] - y0;

            int abxac = abx * acy;
            int abxad = abx * ady;

            // 还有一种两者均等于0的情况（共线）不需要考虑
            // TODO:一些边界情况还存在问题
            if((abxac < 0 && abxad > 0) || (abxac > 0 && abxad < 0) || (abxac == 0 || abxad == 0))
            {
                int miny = (f.content.polygon.y[i] < f.content.polygon.y[i+1])?f.content.polygon.y[i]:f.content.polygon.y[i+1];
                int maxy = (f.content.polygon.y[i] > f.content.polygon.y[i+1])?f.content.polygon.y[i]:f.content.polygon.y[i+1];
                if(!(miny <= y0 && y0 <= maxy)) continue;

                int tx0 = double((y0 - f.content.polygon.y[i]) * (f.content.polygon.x[i+1] - f.content.polygon.x[i]))/(f.content.polygon.y[i+1] - f.content.polygon.y[i]) + f.content.polygon.x[i];
                if(tx0 >= x0)
                {
                    //qDebug() << "Line" << i;
                    incount++;
                }
            }
            /*if(f.content.polygon.y[i] != f.content.polygon.y[i+1])
            {
                int miny = (f.content.polygon.y[i] < f.content.polygon.y[i+1])?f.content.polygon.y[i]:f.content.polygon.y[i+1];
                int maxy = (f.content.polygon.y[i] > f.content.polygon.y[i+1])?f.content.polygon.y[i]:f.content.polygon.y[i+1];
                if(!(miny <= y0 && y0 <= maxy)) continue;
                int tx0 = double((y0 - f.content.polygon.y[i]) * (f.content.polygon.x[i+1] - f.content.polygon.x[i]))/(f.content.polygon.y[i+1] - f.content.polygon.y[i]) + f.content.polygon.x[i];
                if(tx0 >= x0)
                {
                    //qDebug() << "Line" << i;
                    incount++;
                }
            }*/
        }

        if(incount%2 == 1)
        {
            //qDebug() << "Point in the Polygon.";
            return true;
        }
        else
        {
            //qDebug() << "Point out of the Polygon.";
            return false;
        }
    }
    else
    {
        // 此函数只用来判断多边形
        assert(0);
        return false;
    }
}

// 选取裁剪图形
void MainWindow::select_clipper()
{
    int len = figures.length();
    for(int i = 0; i < len; i++)
    {
        int xmin = (pstartx < pendx) ? pstartx:pendx;
        int xmax = (pstartx > pendx) ? pstartx:pendx;
        int ymin = (pstarty < pendy) ? pstarty:pendy;
        int ymax = (pstarty > pendy) ? pstarty:pendy;
        if(figures[i].state == Line)
        {
            liang_barsky_clipper(xmin, ymin, xmax, ymax,
                                 figures[i].content.line.x1, figures[i].content.line.y1,
                                 figures[i].content.line.x2, figures[i].content.line.y2, i);
            this->update();
        }
        else if(figures[i].state == Polygon)
        {
            Sutherland_Hodgman_clipper(xmin, ymin, xmax, ymax, i);
            this->update();
        }
    }
}

void MainWindow::liang_barsky_clipper(float xmin, float ymin, float xmax, float ymax,
                          float x1, float y1, float x2, float y2, int fno)
{
    // defining variables
    float p1 = -(x2 - x1);
    float p2 = -p1;
    float p3 = -(y2 - y1);
    float p4 = -p3;

    float q1 = x1 - xmin;
    float q2 = xmax - x1;
    float q3 = y1 - ymin;
    float q4 = ymax - y1;

    float posarr[5], negarr[5];
    int posind = 1, negind = 1;
    posarr[0] = 1;
    negarr[0] = 0;

    if ((p1 == 0 && q1 < 0) || (p3 == 0 && q3 < 0)) {
        //outtextxy(80, 80, "Line is parallel to clipping window!");
        return;
    }
    if (p1 != 0) {
      float r1 = q1 / p1;
      float r2 = q2 / p2;
      if (p1 < 0) {
        negarr[negind++] = r1; // for negative p1, add it to negative array
        posarr[posind++] = r2; // and add p2 to positive array
      } else {
        negarr[negind++] = r2;
        posarr[posind++] = r1;
      }
    }
    if (p3 != 0) {
      float r3 = q3 / p3;
      float r4 = q4 / p4;
      if (p3 < 0) {
        negarr[negind++] = r3;
        posarr[posind++] = r4;
      } else {
        negarr[negind++] = r4;
        posarr[posind++] = r3;
      }
    }

    float xn1, yn1, xn2, yn2;
    float rn1, rn2;
    //rn1 = maxi(negarr, negind); // maximum of negative array
    //rn2 = mini(posarr, posind); // minimum of positive array
    float m = 0.0;
    for (int i = 0; i < negind; i++)
      if (m < negarr[i])
        m = negarr[i];
    rn1 = m;
    m = 1.0;
    for (int i = 0; i < posind; i++)
      if (m > posarr[i])
        m = posarr[i];
    rn2 = m;

    if (rn1 > rn2)  { // reject
      //qDebug() << "Line is outside the clipping window!";
      return;
    }

    xn1 = x1 + p2 * rn1;
    yn1 = y1 + p4 * rn1; // computing new points

    xn2 = x1 + p2 * rn2;
    yn2 = y1 + p4 * rn2;

    figures[fno].content.line.x1 = xn1;
    figures[fno].content.line.y1 = yn1;
    figures[fno].content.line.x2 = xn2;
    figures[fno].content.line.y2 = yn2;

    //line(x1, 467 - y1, xn1, 467 - yn1);
    //line(x2, 467 - y2, xn2, 467 - yn2);
}

void MainWindow::Sutherland_Hodgman_clipper(float xmin, float ymin, float xmax, float ymax, int fno)
{
    int nx[200], ny[200];
    int ncount = 0;
    bool cutflag = false;
    for(int i = 0; i < figures[fno].content.polygon.count-1; i++)
    {
        int x1 = figures[fno].content.polygon.x[i];
        int y1 = figures[fno].content.polygon.y[i];
        int x2 = figures[fno].content.polygon.x[i+1];
        int y2 = figures[fno].content.polygon.y[i+1];
        qDebug() << "old points:" << x1 << y1 << x2 << y2;
        // defining variables
        float p1 = -(x2 - x1);
        float p2 = -p1;
        float p3 = -(y2 - y1);
        float p4 = -p3;

        float q1 = x1 - xmin;
        float q2 = xmax - x1;
        float q3 = y1 - ymin;
        float q4 = ymax - y1;

        float posarr[5], negarr[5];
        int posind = 1, negind = 1;
        posarr[0] = 1;
        negarr[0] = 0;

        if ((p1 == 0 && q1 < 0) || (p3 == 0 && q3 < 0)) {
            //outtextxy(80, 80, "Line is parallel to clipping window!");
            continue;
        }
        if (p1 != 0) {
          float r1 = q1 / p1;
          float r2 = q2 / p2;
          if (p1 < 0) {
            negarr[negind++] = r1; // for negative p1, add it to negative array
            posarr[posind++] = r2; // and add p2 to positive array
          } else {
            negarr[negind++] = r2;
            posarr[posind++] = r1;
          }
        }
        if (p3 != 0) {
          float r3 = q3 / p3;
          float r4 = q4 / p4;
          if (p3 < 0) {
            negarr[negind++] = r3;
            posarr[posind++] = r4;
          } else {
            negarr[negind++] = r4;
            posarr[posind++] = r3;
          }
        }

        float xn1, yn1, xn2, yn2;
        float rn1, rn2;
        //rn1 = maxi(negarr, negind); // maximum of negative array
        //rn2 = mini(posarr, posind); // minimum of positive array
        float m = 0.0;
        for (int i = 0; i < negind; i++)
          if (m < negarr[i])
            m = negarr[i];
        rn1 = m;
        m = 1.0;
        for (int i = 0; i < posind; i++)
          if (m > posarr[i])
            m = posarr[i];
        rn2 = m;

        if (rn1 > rn2)  { // reject
          //qDebug() << "Line is outside the clipping window!";
          continue;
        }

        xn1 = x1 + p2 * rn1;
        yn1 = y1 + p4 * rn1; // computing new points

        xn2 = x1 + p2 * rn2;
        yn2 = y1 + p4 * rn2;

        nx[ncount] = xn1 + 0.5;
        ny[ncount] = yn1 + 0.5;
        ncount++;
        nx[ncount] = xn2 + 0.5 + 1;
        ny[ncount] = yn2 + 0.5 + 1;
        ncount++;

        //figures[fno].content.polygon.x[i] = xn1 + 0.5;
        //figures[fno].content.polygon.y[i] = yn1 + 0.5;
        //figures[fno].content.polygon.x[i+1] = xn2 + 0.5;
        //figures[fno].content.polygon.y[i+1] = yn2 + 0.5;

        cutflag = true;

        qDebug() << "new points:" << xn1 << yn1 << xn2 << yn2;
    }
    if(cutflag == false) return;

    // 多边形的围合
    if(nx[ncount-1] != nx[0] || ny[ncount-1] != ny[0])
    {
        nx[ncount] = nx[0];
        ny[ncount] = ny[0];
        ncount++;
    }
    // 删除重复的点
    for(int i = 0; i < ncount-1; i++)
    {
        if(abs(nx[i] - nx[i+1]) <=2 && abs(ny[i] - ny[i+1]) <= 2)
        {
            // 将i+1位删去
            qDebug() << "delete" << nx[i+1] << ny[i+1];
            for(int j = i+1; j < ncount-1; j++)
            {
                nx[j] = nx[j+1];
                ny[j] = ny[j+1];
            }
            ncount--;
            i--;
        }
    }
    Figures npolygon;
    npolygon.state = Polygon;
    npolygon.content.polygon.count = ncount;
    npolygon.color = figures[fno].color;
    npolygon.choose = figures[fno].color;
    npolygon.thiscolor = figures[fno].thiscolor;
    for(int i = 0; i < ncount; i++)
    {
        npolygon.content.polygon.x[i] = nx[i];
        npolygon.content.polygon.y[i] = ny[i];
    }
    figures.erase(figures.begin() + fno);
    figures.push_back(npolygon);
}

void MainWindow::paint_line(int x1, int y1, int x2, int y2, bool bechoose)
{
    // 错误检查
    assert(x1 <= x2);
    // TODO:检查这个修改是否有误
    /*if(x1 > x2)
    {
        int t1 = x1;
        int t2 = y1;
        x1 = x2;
        y1 = y2;
        x2 = t1;
        y2 = t2;
    }*/
    if(x2 == x1) x2 = x1 + 1;

    if(bechoose == true)
    {
        QPainter painter(this);
        painter.drawEllipse(x1 - 2, y1 - 2, 4, 4);
        painter.drawEllipse(x2 - 2, y2 - 2, 4, 4);
        painter.drawEllipse((x1 + x2)/2 - 2, (y1 + y2)/2 - 2, 4, 4);

        // 绘制旋转辅助形状
        painter.drawEllipse((x1 + x2)/2 - 2, (y1 + y2)/2 - 32, 4, 4);
        //定义画笔
        QPen pen(Qt::black,1,Qt::DashLine,Qt::FlatCap,Qt::RoundJoin);
        painter.setPen(pen); //使用画笔
        painter.drawLine((x1+x2)/2, (y1+y2)/2, (x1+x2)/2, (y1+y2)/2 - 30);
    }

    mainpainter->begin(this);

    int x, y, dx, dy, p;
    // 一般的算法仅能完成第一象限k<1时的情况，这里我做一个拓展
    double k = (double)(y2 - y1) / (x2 - x1);
    // area代表k的范围，原始算法仅能处理area3，其他的area需要做投影运算：
    // area = 1: k < -1
    // area = 2: -1 <= k <= 0
    // area = 3: 0 < k <= 1
    // area = 4: k > 1
    int area;
    if(k > 0)
    {
        if(k <= 1) area = 3;
        else area = 4;
    }
    else
    {
        if(k < -1) area = 1;
        else area = 2;
    }

    x = x1; y = y1;

    int nx, ny;
    switch(area)
    {
    case 1: y2 = 2 * y1 - y2; nx = y2 - y1 + x1; ny = x2 + y1 - x1; x2 = nx; y2 = ny; dx = x2 - x1; dy = y2 - y1; break;
    case 2: dx = x2 - x1; dy = y1 - y2; break;
    case 3: dx = x2 - x1; dy = y2 - y1; break;
    case 4: nx = y2 - y1 + x1; ny = x2 + y1 - x1; x2 = nx; y2 = ny; dx = x2 - x1; dy = y2 - y1;break;
    default: assert(0); break;
    }
    p = 2 * dy - dx;

    //qDebug() << k << "area is" << area;

    for(; x <= x2; x++) {
        switch(area)
        {
        case 1: mainpainter->drawPoint(y - y1 + x1, 2 * y1 - (x + y1 - x1)); break;
        case 2: mainpainter->drawPoint(x, 2 * y1 - y); break;
        case 3: mainpainter->drawPoint(x, y); break;
        case 4: mainpainter->drawPoint(y - y1 + x1, x + y1 - x1); break;
        default: assert(0); break;
        }

        //mainpainter->drawPoint(x, y);
        if(p >= 0) {
            y++;
            p += 2 * (dy - dx);
        }
        else {
            p += 2 * dy;
        }
    }

    mainpainter->end();
}

void MainWindow::paint_circle(int x, int y, int r, bool bechoose, bool becolor, QColor col)
{   
    // 染色要在前面，不要盖住后面绘制的东西了
    if(becolor)
    {
        /*fill_point_count = 0;
        fillcolor(fillfigure, col, QPoint(x, y));*/
        QPainter painter(this);
        //定义画刷
        QBrush brush(col);
        painter.setBrush(brush); //使用画笔
        QPoint centre(x, y);
        painter.drawEllipse(centre, r, r);
    }
    // 画圆算法
    int x0 = 0, y0 = r;
    int e = 1 - r;

    mainpainter->begin(this);
    while(x0 <= y0) {
        QPoint *points = new QPoint[8];
        points[0] = QPoint(x0, y0);
        points[1] = QPoint(y0, x0);
        points[2] = QPoint(y0, -x0);
        points[3] = QPoint(x0, -y0);
        points[4] = QPoint(-x0, -y0);
        points[5] = QPoint(-y0, -x0);
        points[6] = QPoint(-y0, x0);
        points[7] = QPoint(-x0, y0);

        int i;
        for(i = 0; i < 8; i++) {
            points[i].rx() += x;
            points[i].ry() += y;
        }
        mainpainter->drawPoints(points, 8);

        if(e < 0) {
            x0 = x0 + 1;
            y0 = y0;
            e = e + 2 * x0 + 3;
        }
        else {
            // e >= 0
            x0 = x0 + 1;
            y0 = y0 - 1;
            e = e + 2 * x0 + 5 - 2 * y0;
        }
    }

    if(bechoose == true)
    {
        mainpainter->drawEllipse(x - 2, y - 2, 4, 4);
        mainpainter->drawEllipse(x - 2, y - r - 2, 4, 4);
    }

    mainpainter->end();
}

void MainWindow::paint_ellipse(int x, int y, int rx, int ry, bool bechoose, bool becolor, QColor col)
{
    if(becolor == true)
    {
        /*fill_point_count = 0;
        fillcolor(fillfigure, col, QPoint(x, y));*/
        QPainter painter(this);
        //定义画刷
        QBrush brush(col);
        painter.setBrush(brush); //使用画刷
        QPoint centre(x, y);
        painter.drawEllipse(centre, rx, ry);
    }
    
    double x0 = 0, y0 = ry;
    double p1 = pow(ry, 2) - pow(rx, 2) * ry + (pow(rx, 2) / 4.0);

    mainpainter->begin(this);
    while(pow(ry, 2) * x0 < pow(rx, 2) * y0) {
        QPoint *points = new QPoint[4];
        points[0] = QPoint(x0, y0);
        points[1] = QPoint(x0, -y0);
        points[2] = QPoint(-x0, -y0);
        points[3] = QPoint(-x0, y0);

        int i;
        for(i = 0; i < 4; i++) {
            points[i].rx() += x;
            points[i].ry() += y;
        }
        mainpainter->drawPoints(points, 4);

        x0++;

        if(p1 < 0) {
            p1 += 2 * pow(ry, 2) * x0 + pow(ry, 2);
        }
        else {
            y0--;
            p1 += 2 * pow(ry, 2) * x0 - 2 * pow(rx, 2) * y0 + pow(ry, 2);
        }
    }

    double p2 = pow(ry,2) * pow((x0 + 0.5), 2) + pow(rx * (y0 - 1), 2) - pow(rx * ry, 2);
    while(y0 >= 0) {
        QPoint *points = new QPoint[4];
        points[0] = QPoint(x0, y0);
        points[1] = QPoint(x0, -y0);
        points[2] = QPoint(-x0, -y0);
        points[3] = QPoint(-x0, y0);

        int i;
        for(i = 0; i < 4; i++) {
            points[i].rx() += x;
            points[i].ry() += y;
        }
        mainpainter->drawPoints(points, 4);

        y0--;
        if(p2 > 0) {
            p2 -= 2 * pow(rx, 2) * y0 + pow(rx, 2);
        }
        else {
            x0++;
            p2 += 2 * pow(ry, 2) * x0 - 2 * pow(rx, 2) * y0 + pow(rx, 2);
        }
    }

    mainpainter->end();

    if(bechoose == true)
    {
        QPainter painter(this);
        painter.drawEllipse(x - 2, y - 2, 4, 4);
        painter.drawEllipse(x - rx - 2, y - ry - 2, 4, 4);
        painter.drawEllipse(x + rx - 2, y + ry - 2, 4, 4);

        painter.drawEllipse(x - 2, y - 32, 4, 4);
        //定义画笔
        QPen pen1(Qt::red,1,Qt::DashLine,Qt::FlatCap,Qt::RoundJoin);
        painter.setPen(pen1); //使用画笔
        painter.drawLine(x - rx, y - ry, x - rx, y + ry);
        painter.drawLine(x + rx, y - ry, x + rx, y + ry);
        painter.drawLine(x - rx, y - ry, x + rx, y - ry);
        painter.drawLine(x - rx, y + ry, x + rx, y + ry);
        QPen pen2(Qt::black,1,Qt::DashLine,Qt::FlatCap,Qt::RoundJoin);
        painter.setPen(pen2);
        painter.drawLine(x, y, x, y - 30);
    }
}

void MainWindow::paint_rectangle(int x1, int y1, int x2, int y2, bool bechoose, bool becolor, QColor col)
{
    // 上
    paint_line(x1, y1, x2, y1, false);
    // 下
    paint_line(x1, y2, x2, y2, false);
    // 左
    paint_line(x1, y1, x1, y2, false);
    // 右
    paint_line(x2, y1, x2, y2, false);

    if(becolor == true)
    {
        QPainter painter(this);
        //定义画刷
        QBrush brush(col);
        painter.setBrush(brush); //使用画刷
        painter.drawRect(x1, y1, x2 - x1, y2 - y1);
    }

    if(bechoose == true)
    {
        mainpainter->begin(this);
        mainpainter->drawEllipse(x1 - 2, y1 - 2, 4, 4);
        mainpainter->drawEllipse(x2 - 2, y2 - 2, 4, 4);
        mainpainter->drawEllipse((x1 + x2)/2 - 2, (y1 + y2)/2 - 2, 4, 4);

        mainpainter->drawEllipse((x1 + x2)/2 - 2, (y1 + y2)/2 - 32, 4, 4);
        mainpainter->end();

        QPainter painter(this);
        QPen pen(Qt::black,1,Qt::DashLine,Qt::FlatCap,Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawLine((x1 + x2)/2, (y1 + y2)/2, (x1 + x2)/2, (y1 + y2)/2 - 30);
    }
}

void MainWindow::paint_polygon(int x[], int y[], int num, bool bechoose, bool becolor, QColor col)
{
    for(int i = 0; i < num - 1; i++)
    {
        if(x[i] <= x[i+1])
            paint_line(x[i], y[i], x[i+1], y[i+1], false);
        else
            paint_line(x[i+1], y[i+1], x[i], y[i], false);
    }

    if(becolor)
    {
        fill_point_count = 0;
        fillcolor(fillfigure, col, QPoint(x[0], y[0]));
    }

    if(bechoose)
    {
        int max_x = 0, max_y = 0, min_x = 10000, min_y = 10000;
        for(int i = 0; i < num; i++)
        {
            max_x = (max_x > x[i])?max_x:x[i];
            min_x = (min_x < x[i])?min_x:x[i];
            max_y = (max_y > y[i])?max_y:y[i];
            min_y = (min_y < y[i])?min_y:y[i];
        }
        QPainter painter1(this);
        //定义画笔
        QPen pen1(Qt::red,1,Qt::DashLine,Qt::FlatCap,Qt::RoundJoin);
        painter1.setPen(pen1); //使用画笔
        painter1.drawLine(min_x, min_y, max_x, min_y);
        painter1.drawLine(min_x, min_y, min_x, max_y);
        painter1.drawLine(max_x, min_y, max_x, max_y);
        painter1.drawLine(min_x, max_y, max_x, max_y);

        QPen pen2(Qt::black,1,Qt::DashLine,Qt::FlatCap,Qt::RoundJoin);
        painter1.setPen(pen2);
        // 为图形旋转设置的辅助图形
        painter1.drawLine((max_x+min_x)/2, (max_y+min_y)/2, (max_x+min_x)/2, (max_y+min_y)/2 - 30);

        QPainter painter2(this);
        for(int i = 0; i < num; i++)
        {
            painter2.drawEllipse(x[i]-2, y[i]-2, 4, 4);
        }
        painter2.drawEllipse((max_x + min_x)/2-2, (max_y + min_y)/2-2, 4, 4);
        painter2.drawEllipse((max_x + min_x)/2-2, (max_y + min_y)/2-32, 4, 4);
        painter2.drawEllipse(min_x - 2, min_y - 2, 4, 4);
        painter2.drawEllipse(max_x - 2, max_y - 2, 4, 4);
    }
}

void MainWindow::on_action_Save_triggered()
{
    //调用QfileDialog的保存对话框，获取文件名
    QString filename = QFileDialog::getSaveFileName(this,tr("Save Image"),"",tr("Images (*.png *.bmp *.jpg)")); //选择路径
    // 构造保存图片的大小
    QSize opsize(this->size());
    // 使用QPixmap来存储图片
    QPixmap oppicture(opsize);
    // 将当前画布上的像素信息转存到QPixmap上
    this->render(&oppicture);
    //调用QPixmap的方法存储图片
    oppicture.save(filename);
}

void MainWindow::on_action_Load_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, QStringLiteral("Open picture"), "", QStringLiteral("picture (*.jpg;*.png;*.bmp);;All (*.*)"));
    if(!filename.isEmpty())
    {
        qDebug() << "Open Successful." << filename;
        QImage img;
        if(!(img.load(filename))) //加载图像
        {
           QMessageBox::information(this, tr("Open image successfully"),tr("open image unsuccessfully"));
           return;
        }
        ui->label->resize(img.width(),img.height());
        ui->label->setPixmap(QPixmap::fromImage(img.scaled(ui->label->size())));
    }
    else
    {
        qDebug() << "Open Unsuccessful.";
    }
}

void MainWindow::on_actionOpen3D_triggered()
{
    QString exename = "openGLdll/open3D.exe";
    QString filename = QFileDialog::getOpenFileName(this, QStringLiteral("Open .off file"), "offfiles/", QStringLiteral("picture (*.off);;All (*.*)"));
    if(!filename.isEmpty())
    {
        QProcess *process = new QProcess;
        QStringList str;
        str.append(filename);
        process->start(exename, str);
    }
}

void MainWindow::on_actionInitialize_triggered()
{
    figures.clear();
    oldfigures.clear();
    this->update();
}

void MainWindow::on_actionCancel_All_States_triggered()
{
    statepainter = Free;
    statechange = Free;
    curstate = 0;
}

void MainWindow::on_actionHow_to_use_triggered()
{
    QMessageBox::about(this, "Tell me how to use", \
                       "Welcome to my painting system.\n"
                       "Use Menu Bar to begin your work:\n\n"
                       "File - Save: Save your painting with the .jpg/.png/.bmp suffix.\n"
                       "File - Load: Load a picture with the .jpg/.png/.bmp suffix to the board.\n"
                       "File - Open3D: Open a file with the .off suffix and operate on it.\n\n"
                       "Draw - Line/Circle/Ellipse/Rectangle: Click your mouse and drag, then release to paint.\n"
                       "Draw - Polygon: Click the mouse n times to determine the position of the polygon n points, click the start point to end the drawing\n"
                       "Draw - Quadratic Bezier curves/ Cubic Bezier curves: Click the mouse 3 or 4 times to determine the position of the control points of this curves.\n\n"
                       "Tools - Cut: Cut lines or polygons.\n"
                       "Tools - Color: To color a graph, click on the inside of the graph.\n"
                       "Tools - Change Color: Change the palette.\n"
                       "Tools - Remove: Click on the boundary of a graph to delete the graph.\n\n"
                       "In addition, you can click on a graph to display its transformation points (rotation, translation, scaling, etc.).\n"
                       "Let's try it!\n");
}
