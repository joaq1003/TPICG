#include <forward_list>
#include <iostream>
#include <cmath>
#ifdef __APPLE__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/glut.h>
#endif
#include "raster_algs.hpp"
#include <algorithm>
#include "OSD.hpp"
using namespace std;
void draw_line(Point p0, Point p1, bool antialiasing) {

    /*=======================ALGORITMO DDA - NO ANTIALIASING=======================*/ 
    
    glBegin(GL_POINTS); //empezamos a pintar los puntos
    if(!antialiasing){
    float  dx = p1.x - p0.x; 
    float  dy = p1.y - p0.y;
    
    if (abs(dx)>= abs(dy)){ // horizontal (>=, ==> ambos pueden ser 0)
        
        if(p0.x > p1.x) {
            Point p = p0;
            p0 = p1;
            p1 = p;
            
        }
        
        float x=redon(p0.x), 
              x1=redon(p1.x),
              y=redon(p0.y);
        
        glVertex2f(x,y); //pintamos el primer punto
        
        if (x1 == x) return; // pinta uno y sale si dx ~0
        
        float m=dy/dx; // dx no es nulo pero puede ser muy chico
        
        while((++x)<x1) {
            y+=m; 
            glVertex2f(x,redon(y)); //pinta los puntos hasta el anterior al x1
        }
    }
    else { // vertical (|dy|>|dx| ==> dy no es nulo)
        if(p0.y > p1.y) {
            Point p = p0;
            p0 = p1;
            p1 = p;
            
        }
        
        float y=redon(p0.y), 
              y1=redon(p1.y),
              x=redon(p0.x);
        
        glVertex2f(x,y);//pintamos el primer punto
        
        if (y1 == y) return; // pinta uno y sale si dy ~0
        
        float m=dx/dy; // dy no es nulo pero puede ser muy chico
        
        while((++y)<y1) {
            x+=m; 
            glVertex2f(redon(x),y); //pinta los puntos hasta el anterior al x1
        }
    }
    }else{
    
    /*=======================ALGORITMO XiaolinWu - ANTIALIASING=======================*/
    
    /*Este algoritmo es una mejora del algoritmo de Bresenham que permite dibujar rectas
    reduciendo el aliasing. El algoritmo se basa en dibujar parejas rectas de pixeles a
    lo largo del trazado de la recta con diferentes intensidades en funcion de la
    recta real*/
    float  dx = p1.x - p0.x; 
    float  dy = p1.y - p0.y;
    
    if(abs(dx) >= abs(dy)){ // Horizontal o inclinada
        if(p0.x > p1.x){
            std::swap(p0,p1);
            dx = -dx;
            dy = -dy;
        }
        float gradiente = dy / dx;
        float y = ceil(p0.y);
        
        for(int x = floor(p0.x); x <= floor(p1.x); x++) {
            float intensidad1 = 1 - (y - floor(y));
            float intensidad2 = 1 - intensidad1;
            
            glColor4f(1.f,0.f,0.f, intensidad1);
            glVertex2f(redon(x),redon(y) - 1);
            
            glColor4f(1.f,0.f,0.f, intensidad2);
            glVertex2f(redon(x),redon(y));
            
            y += gradiente;
        }
    }else{
        if(p0.y > p1.y){
            swap(p0,p1);
            dx = -dx; 
            dy = -dy;
        }
        float gradiente = dx / dy;
        float x = floor(p0.x);
        
        for(int y = ceil(p0.y); y <= ceil(p1.y); y++){
            float intensidad1 = 1 - (x - floor(x));
            float intensidad2 = 1 - intensidad1;
            
            glColor4f(1.f,0.f,0.f, intensidad1);
            glVertex2f(redon(x), redon(y));
            
            glColor4f(1.f,0.f,0.f, intensidad2);
            glVertex2f(redon(x) + 1, redon(y));
            x += gradiente;
        }
    }
    
    }
    
    glEnd();

}

//
//void draw_curve(curve_func_t f) {
//    glBegin(GL_POINTS);
//    
//    /// @TODO: implementar algun algoritmo de rasterizacion de curvas
//    
//    float t = 0, t_ant, t_aux;
//    auto r = f(t);
//    
//    glVertex2f(redon(r.p.x),redon(r.p.y)); //pintamos el primer punto
//    
//    t_ant = t;
//    t += 1/max(abs(r.d.x),abs(r.d.y)); //actualizamos el t
//    
//    while(t < 1) {
//        
//        auto r_ant = r; //guardamos el anterior
//        
//        r = f(t); //actualizamos el punto y derivada
//
//        
//        bool cond_x = abs(redon(r.p.x) - redon(r_ant.p.x)) >= 2; // verificamos si la dy pasa 1
//        bool cond_y = abs(redon(r.p.y) - redon(r_ant.p.y)) >= 2; // verificamos que dy pase 1
//        
//        if(cond_x or cond_y) {
//            
//            glColor4f(1.0f, 0.2f, 0.2f, 0.9f);
//            t_aux = (t_ant+t)/2; // promediamos el t anterior y el t actual
//            auto r_aux = f(t_aux);
//            glVertex2f(redon(r_aux.p.x),redon(r_aux.p.y)); //pintamos el punto rojo
//            glColor4f(0.2f, 0.2f, 1.0f, 0.6f);
//            
//        } 
//        
//        
//        if( (redon(r_ant.p.x) != redon(r.p.x) ) or ( redon(r_ant.p.y) != redon(r.p.y)) ) { //verificamos que no se pisen los pixeles
//           glVertex2f(redon(r.p.x),redon(r.p.y)); //pintamos el punto
//        }
//        
//        t_ant = t;
//        t += 1/max(abs(r.d.x),abs(r.d.y)); //actualizamos el t
//        
//    }
//    
//    // f es la función de la curva, recibe un t entre 0 y 1 y retorna
//    // el punto y la derivada en ese punto, por ej:
//    //    auto r = f(0.5); // calcular en el punto "medio" de la curva
//    // entonces r.p es el punto (tipo Point) y r.d la derivada (tipo Vector)
//    
//    glEnd();
//}






