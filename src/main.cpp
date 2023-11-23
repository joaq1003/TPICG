#include <cstdlib>
#include <iostream>
#include <csignal>
#include <ctime>
#include <algorithm>
#ifdef __APPLE__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/glut.h>
#endif
#include "raster_algs.hpp"

#include <GL/gl.h>
#include "OSD.hpp"
using namespace std;

// ids de glut para identificar cada ventana
static int main_win_id = -1, zoom_win_id = -1;

// tamaños de las ventanas
//static Point main_win_size = {800,600}, zoom_win_size = {380,600};
float win_w=650,win_h=650;

static Point main_win_size = {win_w,win_h}, zoom_win_size = {win_w,win_h};
static float // colores
    color_texto[]={.3f,0.2f,0.8f}, // osd
    color_fondo[]={1,1,1};       // color de fondo
        
// puntos para definir una curva (los 4 primeros) 
// y varios segmentos (del 5to en adelante, de a pares)
static std::vector<Point> vpoints;

// factor de escala para el zoom
static int zoom_factor = 10;

// posición del zoom en la ventana original
static Point zoom_p0 = {270,270}; // en las rectas
//static Point zoom_p0 = {100,140}; // en la curva

// indice del pto seleccionado, y valores especiales 
// para no seleccion o para arrastre del area de zoom
static constexpr int no_selection = -1;
static constexpr int zoom_selection = -2;
static int selection = no_selection;

// grado de la curva
static constexpr int curve_degree = 3;
static bool antialiasing = false;
// funciones de la curva... se define por los puntos vp, y 
// dado un t, obtiene el punto y la derivada en el punto
static curve_ret_t bezier(float t, Point vp[], int degree, int iter=1) {
    if (iter==degree) {
        return { lerp(vp[0],vp[1],t),
                 { degree*(vp[1].x-vp[0].x),
                   degree*(vp[1].y-vp[0].y) } };
    }
    for(size_t i=0;i<=degree-iter;i++)
        vp[i] = lerp(vp[i],vp[i+1],t);
    return bezier(t,vp,degree,++iter);
}
static curve_ret_t curve_func(float t) {
    static Point v[curve_degree+1];
    std::copy(vpoints.begin(),vpoints.begin()+curve_degree+1,v);
    return bezier(t,v,curve_degree); 
}

// colores
static float color_segs_int[]  = { 1.f, 0.0f, 0.0f, 1.f };
static float color_segs_ext[]  = { 0.8f, 0.0f, 0.0f, 1.0f };
static float color_segs_ref[]  = { 0.0f, 0.7f, 0.0f, 1.0f };
static float color_curve_int[] = { 0.2f, 0.2f, 1.0f, 0.6f };
static float color_curve_ext[] = { 0.2f, 0.0f, 0.8f, 0.5f };
static float color_curve_pol[] = { 0.0f, 0.0f, 0.0f, 0.2f };


//------------------------------------------------------
//         callbacks comunes a ambas ventanas
//------------------------------------------------------

void common_display() {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
    
    glutSetWindow(main_win_id);
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);

    // poligono de control de la curva
//    glColor4fv(color_curve_pol);
//    glBegin(GL_LINE_STRIP); glLineWidth(1);
//    for(int i=0,s=std::min(int(vpoints.size()),curve_degree+1);i<s;i++) { 
//        glVertex2f(vpoints[i].x,vpoints[i].y);
//    }
//    glEnd();
    
    glPointSize(1);
    // curva de bezier
//    glColor4fv(color_curve_int); 
//    if (vpoints.size()>=curve_degree+1) draw_curve(curve_func);
    // segmentos
    glColor4fv(color_segs_int); 
    for(size_t i=curve_degree+1;i+1<vpoints.size();i+=2) draw_line(vpoints[i],vpoints[i+1],antialiasing);
    
    // extremos y puntos de control
    glPointSize(3); 
//    glColor4fv(color_curve_ext);
    glBegin(GL_POINTS);
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
    for(size_t i=4;i<vpoints.size();i++) {
        if (i==curve_degree+1) glColor4fv(color_segs_ext);
        glVertex2f(vpoints[i].x,vpoints[i].y);
    }
    glEnd();
    
    
    // zoom
    if (zoom_win_size) {
        OSD.SetMode(OSD_t::PersistentMode);
        OSD<<"(presione A para aplicar antialiasing)"<<'\n';
        int zw = zoom_win_size.x/zoom_factor+1, 
            zh = zoom_win_size.y/zoom_factor+1;
        
        // capturar el contenido de la ventana principal
        static std::vector<GLfloat> buffer;
        for(auto &x:buffer) x=0; // negro si el zoom se sale de la ventana
        buffer.resize(zw*zh*4,0);
        glReadBuffer(GL_BACK);
        glReadPixels(zoom_p0.x,zoom_p0.y,zw,zh,GL_RGBA,GL_FLOAT,buffer.data()); // lee el z-buffer
        
        // mostrar la captura aumentada en la ventana de zoom
        glutSetWindow(zoom_win_id);
        glPixelZoom(zoom_factor,zoom_factor);
        glDrawPixels(zw,zh,GL_RGBA,GL_FLOAT,buffer.data()); // los valores pasan al color buffer como luminancias
        
        if (zoom_factor>5) {
            // dibujar la grilla de pixels
            glLineWidth(1);
            glColor3f(.5,.5,.5);
            glBegin(GL_LINES);
            for(int i=0;i<zw;++i) {
                glVertex2i(i*zoom_factor,0);
                glVertex2i(i*zoom_factor,zoom_win_size.y);
            }
            for(int j=0;j<zh;++j) {
                glVertex2i(0,j*zoom_factor);
                glVertex2i(zoom_win_size.x,j*zoom_factor);
            }
            glEnd();
            
            // dibujar las lineas sobre el zoom
            glLineWidth(1+zoom_factor/10); 
            glColor3fv(color_segs_ref);
            glBegin(GL_LINES);
            for(size_t i=4;i<2*(vpoints.size()/2);i++)
                glVertex2f(
                           zoom_factor*(vpoints[i].x-int(zoom_p0.x)/*-.5*/),
                           zoom_factor*(vpoints[i].y-int(zoom_p0.y)/*+.5*/)
                           );
            glEnd();
        }
        
        glutSwapBuffers();
        
        glutSetWindow(main_win_id);
        
        // marcar el area del zoom en la ventana principal
        glColor4f(0,0,0,.35);
        glBegin(GL_LINE_LOOP);
        glVertex2d(zoom_p0.x,zoom_p0.y);
        glVertex2d(zoom_p0.x+zw,zoom_p0.y);
        glVertex2d(zoom_p0.x+zw,zoom_p0.y+zh);
        glVertex2d(zoom_p0.x,zoom_p0.y+zh);
        glEnd();
        
        glPointSize(5);
        glBegin(GL_POINTS);
        glVertex2i(zoom_p0.x,zoom_p0.y);
        glEnd();
        
    }
    
    glutSwapBuffers();
}

Point common_reshape(int w, int h) {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
    if (w!=0&&h!=0) {
        glViewport(0,0,w,h);
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        gluOrtho2D(0,w,0,h);
        glMatrixMode (GL_MODELVIEW);
        glLoadIdentity();
    }
    return {float(w),float(h)};
}


//------------------------------------------------------
//         callbacks de la ventana principal
//------------------------------------------------------

static float distance(Point a, Point b) {
	int dx = a.x-b.x, dy = a.y-b.y;
	return sqrt(dx*dx+dy*dy);
}

bool set_selection(Point r) {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
    static constexpr int selection_tolerance = 10;
	selection = no_selection;
	float min = 0, aux = distance(r,zoom_p0);
	if (aux<selection_tolerance) {
		selection = zoom_selection;
		min = aux;
	}
	for(size_t i=0;i<vpoints.size();i++) { 
		aux = distance(vpoints[i],r);
		if (aux<selection_tolerance && (selection==no_selection || aux<min)) {
			min = aux;
			selection = i;
		}
	}
	return selection!=no_selection;
}

void main_motion(int x, int y) {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
	y = main_win_size.y-y;
	if (selection==no_selection) return;
	if (selection==zoom_selection) {
		zoom_p0.x = x+.5f;
		zoom_p0.y = y+.5f;
	} else {
		vpoints[selection].x = redon(x);
		vpoints[selection].y = redon(y);
	}
	glutPostRedisplay();
}

void main_mouse(int but, int sta, int x, int y) {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
	y = main_win_size.y-y;
	if (sta==GLUT_DOWN) {
		Point p = {redon(x),redon(y)};
		if (!set_selection(p)) {
			if (but==GLUT_LEFT_BUTTON) {
				selection = vpoints.size();
				vpoints.push_back(p);
				glutPostRedisplay();
			}
			return;
		} else if (but==GLUT_RIGHT_BUTTON) {
			if (vpoints.size()%2==1) vpoints.pop_back();
			if (selection!=vpoints.size() && selection>curve_degree) {
				selection = 2*(selection/2);
				vpoints.erase(vpoints.begin()+selection,
							  vpoints.begin()+selection+2);
			}
			selection = no_selection;
			glutPostRedisplay();
		}
	}
}


void main_reshape (int w, int h) {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
	main_win_size = common_reshape(w,h);
}

void main_keyboard(unsigned char key, int x, int y) {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
        cout<<key<<endl;
    switch(tolower(key)) {
        case 27:
            exit(0);
        case 'a':
            antialiasing = !antialiasing;
            OSD.SetMode(OSD_t::PersistentMode);
            OSD<<"(presione A para aplicar antialiasing)"<<'\n';
            break;
    default:
        ;
    }
    glutPostRedisplay();
}


//------------------------------------------------------
//         callbacks de la ventana de zoom
//------------------------------------------------------


static bool zoom_moving = false;
Point zoom_moving_p0;
Point zoom_moving_base;


void zoom_motion(int x, int y) {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
	if (!zoom_moving) return;
	zoom_p0.x = zoom_moving_p0.x + (zoom_moving_base.x-x)/zoom_factor;
	zoom_p0.y = zoom_moving_p0.y - (zoom_moving_base.y-y)/zoom_factor;
	glutPostRedisplay();
}

void zoom_mouse(int but, int sta, int x, int y) {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
	if (sta!=GLUT_DOWN) { zoom_moving = false; return; }
	if (but==GLUT_LEFT) { 
		zoom_moving = true;
		zoom_moving_p0 = zoom_p0;
		zoom_moving_base = {float(x),float(y)};
	}
	if (but!=4 && but!=3) return;
	if (but==3) ++zoom_factor;
	else if (zoom_factor>2) --zoom_factor;
	glutSetWindow(main_win_id);
	glutPostRedisplay();
}

void zoom_reshape(int w, int h) {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
	zoom_win_size = common_reshape(w,h);
}



//------------------------------------------------------
//         main e inicializacion general
//------------------------------------------------------

static void initialize() {
    
    // main window
   
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
    glutInitDisplayMode (GLUT_RGBA|GLUT_DOUBLE);
	glutInitWindowSize (int(main_win_size.x),int(main_win_size.y));
    glutInitWindowPosition(0,0);
	glutCreateWindow ("Area de trabajo");
	main_win_id = glutGetWindow();
 
    glClearColor(1.f,1.f,1.f,1.f);
    
    glutDisplayFunc(common_display);
    glutMouseFunc(main_mouse);
    glutMotionFunc(main_motion);
    glutReshapeFunc(main_reshape);
    glutKeyboardFunc(main_keyboard);
    glEnable(GL_BLEND);
    
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    // zoom window
    
	glutInitWindowSize (int(zoom_win_size.x),int(zoom_win_size.y));
	glutInitWindowPosition (int(main_win_size.x),0);
	glutCreateWindow ("Zoom");
	zoom_win_id = glutGetWindow();
    
    glutSetWindow(zoom_win_id);
    glClearColor(1.f,1.f,1.f,1.f);
    glutDisplayFunc(common_display);
    glutReshapeFunc(zoom_reshape);
    glutMouseFunc(zoom_mouse);
    glutMotionFunc(zoom_motion);
    
    // curva inicial
    vpoints.push_back({ 60.5f, 60.5f});
    vpoints.push_back({100.5f,310.5f});
    vpoints.push_back({200.5f, 70.5f});
    vpoints.push_back({498.5f,150.5f});
    
    // segmentos iniciales
    constexpr float pi = 4.f*atan(1), r = 17;
    Point c = {300.f, 300.f};
    for(int i=0;i<10;i++) {
        float ang = i*(2*pi)/10;
        Vector p = {std::cos(ang), std::sin(ang) };
        vpoints.push_back(redon(c+r*p));
        vpoints.push_back(redon(c+r*p*10));
    }
    glColor3fv(color_texto);
    OSD.Render(win_w,win_h);
}

int main (int argc, char **argv) {
    OSD.SetMode(OSD_t::PersistentMode);
    OSD<<"(presione A para aplicar antialiasing)"<<'\n';
	glutInit (&argc, argv);
    
	initialize();
	glutMainLoop();
}
