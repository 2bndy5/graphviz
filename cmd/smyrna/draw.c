/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**********************************************************
*      This software is part of the graphviz package      *
*                http://www.graphviz.org/                 *
*                                                         *
*            Copyright (c) 1994-2004 AT&T Corp.           *
*                and is licensed under the                *
*            Common Public License, Version 1.0           *
*                      by AT&T Corp.                      *
*                                                         *
*        Information and Software Systems Research        *
*              AT&T Research, Florham Park NJ             *
**********************************************************/

/*

XDOT DRAWING FUNCTIONS, maybe need to move them somewhere else
		for now keep them at the bottom
*/
#include "draw.h"
#include "topview.h"
#include "colorprocs.h"
#include "glutils.h"
#include "math.h"
#include "selection.h"
#include "xdot.h"
#include "viewport.h"
#include "topfisheyeview.h"

//delta values
static float dx = 0.0;
static float dy = 0.0;
static float globalz = 0.0;

GLubyte rasters[24] = {
    0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xff, 0x00,
	0xff, 0x00,
    0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xff, 0xc0, 0xff, 0xc0
};

static void
DrawBezier(GLfloat * xp, GLfloat * yp, GLfloat * zp, int filled, int param)
{
    /*copied from NEHE */
    /*Written by: David Nikdel ( ogapo@ithink.net ) */
    double Ax = xp[0];
    double Ay = yp[0];
    double Az = zp[0];
    double Bx = xp[1];
    double By = yp[1];
    double Bz = zp[1];
    double Cx = xp[2];
    double Cy = yp[2];
    double Cz = zp[2];
    double Dx = xp[3];
    double Dy = yp[3];
    double Dz = zp[3];
    double X;
    double Y;
    double Z;
    int i = 0;			//loop index
    // Variable
    double a = 1.0;
    double b = 1.0 - a;
    /* Tell OGL to start drawing a line strip */
    glLineWidth(view->LineWidth);
    if (!filled) {

	if (param == 0)
	    glColor4f(view->penColor.R, view->penColor.G, view->penColor.B,
		      view->penColor.A);
	if (param == 1)		//selected
	    glColor4f(view->selectedNodeColor.R, view->selectedNodeColor.G,
		      view->selectedNodeColor.B,
		      view->selectedNodeColor.A);
	glBegin(GL_LINE_STRIP);
    } else {
	if (param == 0)
	    glColor4f(view->fillColor.R, view->fillColor.G,
		      view->fillColor.B, view->penColor.A);
	if (param == 1)		//selected
	    glColor4f(view->selectedNodeColor.R, view->selectedNodeColor.G,
		      view->selectedNodeColor.B,
		      view->selectedNodeColor.A);
	glBegin(GL_POLYGON);
    }
    /* We will not actually draw a curve, but we will divide the curve into small
       points and draw a line between each point. If the points are close enough, it
       will appear as a curved line. 20 points are plenty, and since the variable goes
       from 1.0 to 0.0 we must change it by 1/20 = 0.05 each time */
    for (i = 0; i <= 20; i++) {
	// Get a point on the curve
	X = Ax * a * a * a + Bx * 3 * a * a * b + Cx * 3 * a * b * b +
	    Dx * b * b * b;
	Y = Ay * a * a * a + By * 3 * a * a * b + Cy * 3 * a * b * b +
	    Dy * b * b * b;
	Z = Az * a * a * a + Bz * 3 * a * a * b + Cz * 3 * a * b * b +
	    Dz * b * b * b;
	// Draw the line from point to point (assuming OGL is set up properly)
	glVertex3d(X, Y, Z+globalz);
	// Change the variable
	a -= 0.05;
	b = 1.0 - a;
    }
// Tell OGL to stop drawing the line strip
    glEnd();
}

static void set_options(sdot_op * op, int param)
{

    if ((param == 1) && (view->mouse.mouse_mode == 10) && (view->mouse.mouse_down == 1))	//selected, if there is move, move it
    {
		dx = view->GLx - view->GLx2;
		dy = view->GLy - view->GLy2;
    } else 
	{
		dx = 0;
		dy = 0;
    }

}

static void relocate_spline(sdot_op * sop, int param)
{
/*    Agedge_t *e;
    Agnode_t *tn;		//tail node
    Agnode_t *hn;		//head node
    int i = 0;
    xdot_op *op = &sop->op;
    if (AGTYPE(sop->obj) == AGEDGE)
	{
		e = sop->obj;
		tn = agtail(e);
		hn = aghead(e);
		if ((OD_Selected(hn) == 1) && (OD_Selected(tn) == 0))
		{
			set_options(sop, 1);
			for (i = 1; i < op->u.bezier.cnt - 1; i = i + 1)
			{
				if ((dx != 0) || (dy != 0)) 
				{
					op->u.bezier.pts[i].x =
							op->u.bezier.pts[i].x -
								(int) (dx * (float) i /
									(float) (op->u.bezier.cnt));
					op->u.bezier.pts[i].y =
								op->u.bezier.pts[i].y -
									(int) (dy * (float) i /
								       (float) (op->u.bezier.cnt));
				}
		    }
			if ((dx != 0) || (dy != 0)) 
			{
				op->u.bezier.pts[op->u.bezier.cnt - 1].x =
						op->u.bezier.pts[op->u.bezier.cnt - 1].x - (int) dx;
				op->u.bezier.pts[op->u.bezier.cnt - 1].y =
						op->u.bezier.pts[op->u.bezier.cnt - 1].y - (int) dy;
			}
		}
		else if ((OD_Selected(hn) == 0) && (OD_Selected(tn) == 1))
		{
			set_options(sop, 1);
			for (i = op->u.bezier.cnt - 1; i > 0; i = i - 1) 
			{
				if ((dx != 0) || (dy != 0)) 
				{
					op->u.bezier.pts[i].x =
						op->u.bezier.pts[i].x -
							(int) (dx * (float) (op->u.bezier.cnt - i) /
											(float) (op->u.bezier.cnt));
				op->u.bezier.pts[i].y =
						op->u.bezier.pts[i].y -
							(int) (dy * (float) (op->u.bezier.cnt - i) /
								(float) (op->u.bezier.cnt));
				}
			}
			if ((dx != 0) || (dy != 0)) 
			{
				op->u.bezier.pts[0].x = op->u.bezier.pts[0].x - (int) dx;
				op->u.bezier.pts[0].y = op->u.bezier.pts[0].y - (int) dy;
			}
		}
		else if ((OD_Selected(hn) == 1) && (OD_Selected(tn) == 1)) 
		{
		    set_options(sop, 1);
			for (i = 0; i < op->u.bezier.cnt; i = i + 1) 
			{
				if ((dx != 0) || (dy != 0)) 
				{
					op->u.bezier.pts[i].x =
							op->u.bezier.pts[i].x - (int) dx;
					op->u.bezier.pts[i].y =
							op->u.bezier.pts[i].y - (int) dy;
				}
			}
		}
	}*/
}

static void DrawBeziers(xdot_op * op, int param)
{
    //SEND ALL CONTROL POINTS IN 3D ARRAYS

    GLfloat tempX[4];
    GLfloat tempY[4];
    GLfloat tempZ[4];
    int temp = 0;
    int filled;
    int i = 0;
//    SelectBeziers((sdot_op *) op);
    relocate_spline((sdot_op *) op, param);
    if (op->kind == xd_filled_bezier)
	filled = 1;
    else
	filled = 0;

    for (i = 0; i < op->u.bezier.cnt; i = i + 1) {
	if (temp == 4) {
	    DrawBezier(tempX, tempY, tempZ, filled, param);
	    tempX[0] = (GLfloat) op->u.bezier.pts[i - 1].x;
	    tempY[0] = (GLfloat) op->u.bezier.pts[i - 1].y;
	    tempZ[0] = (GLfloat) op->u.bezier.pts[i - 1].z;
	    temp = 1;
	    tempX[temp] = (GLfloat) op->u.bezier.pts[i].x;
	    tempY[temp] = (GLfloat) op->u.bezier.pts[i].y;
	    tempZ[temp] = (GLfloat) op->u.bezier.pts[i].z;
	    temp = temp + 1;
	} else {
	    tempX[temp] = (GLfloat) op->u.bezier.pts[i].x;
	    tempY[temp] = (GLfloat) op->u.bezier.pts[i].y;
	    tempZ[temp] = (GLfloat) op->u.bezier.pts[i].z;
	    temp = temp + 1;
	}
    }
    DrawBezier(tempX, tempY, tempZ, filled, param);
}

/*function to load .raw files*/
#if 0
static void
load_raw_texture(char *file_name, int width, int height, int depth,
		 GLenum colour_type, GLenum filter_type)
{
    //Line 3 creates a pointer to an (as yet unallocated) array of gl unsigned bytes - 
    //this will store our image until we've passed it to opengl.  Line 4 stores the file pointer, don't worry about that. 
    GLubyte *raw_bitmap;
    FILE *file;

    //The if statement from line 6-10 opens the file and reports an error if it doesn't exist.   

    if ((file = fopen(file_name, "rb")) == NULL) {
	printf("File Not Found : %s\n", file_name);
	exit(1);
    }
    //Line 11 allocates the correct number of bytes for the size and depth of the image.  Remember, our image depth will usually be 3 -- one 'channel' each for red, green and blue values.   
    //Lines 13-18 check if the memory was allocated correctly and quit the program if there was a problem. 

    raw_bitmap = N_GNEW(width * height * depth, GLubyte);

    if (raw_bitmap == NULL) {
	printf("Cannot allocate memory for texture\n");
	fclose(file);
	exit(1);
    }
    //Line 19 reads the required number of bytes from the file and places them into our glubyte array.  Line 20 closes the close as it isn't required anymore.   
    fread(raw_bitmap, width * height * depth, 1, file);
    fclose(file);

    //Lines 22-25 set the texture's mapping type and environment settings.  GL_TEXTURE_MAG_FILTER and GL_TEXTURE_MIN_FILTER are enumerands that let us change the way in which opengl magnifies and minifies the texture.  If we passed GL_LINEAR to the function, our texture would be interpolated using bilinear filtering (in other words, it'd appear smoothed).  If we passed GL_NEAREST then no smoothing would occur.  By passing GL_MODULATE to the texture environment function, we tell opengl to blend the texture with the base colour of the object.  Had we specified GL_DECAL or GL_REPLACE then the base colour (and therefore our lighting effect) would be replaced purely with the colours of the texture. 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_type);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_type);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    //Line 27 passes the image to opengl using the function gluBuild2DMipmaps.
    //Read a graphics book for a description of mipmaps.
    //This function will also resize the texture map if it doesn't conform 
    //to the opengl restriction of height and width values being a power of 2.   
    gluBuild2DMipmaps(GL_TEXTURE_2D, colour_type, width, height,
		      colour_type, GL_UNSIGNED_BYTE, raw_bitmap);

    //We don't need our texture in the temporary array anymore as it has been passed to opengl.
    //We use the standard C library call 'free' to return the allocated memory.  
    free(raw_bitmap);
}
#endif

//Draws a ellpise made out of points.
//void DrawEllipse(xdot_point* xpoint,GLfloat xradius, GLfloat yradius,int filled)
static void DrawEllipse(xdot_op * op, int param)
{
    //to draw a circle set xradius and yradius same values
    GLfloat x, y, xradius, yradius;
    int i = 0;
    int filled;
    set_options((sdot_op *) op, param);
    x = op->u.ellipse.x - dx;
    y = op->u.ellipse.y - dy;
    xradius = (GLfloat) op->u.ellipse.w;
    yradius = (GLfloat) op->u.ellipse.h;
//    SelectEllipse((sdot_op *) op);
    if (op->kind == xd_filled_ellipse) {
	if (param == 0)
	    glColor4f(view->fillColor.R, view->fillColor.G,
		      view->fillColor.B, view->fillColor.A);
	if (param == 1)		//selected
	    glColor4f(view->selectedNodeColor.R, view->selectedNodeColor.G,
		      view->selectedNodeColor.B,
		      view->selectedNodeColor.A);

	filled = 1;
    } else {
	if (param == 0)
	    glColor4f(view->penColor.R, view->penColor.G, view->penColor.B,
		      view->penColor.A);
	if (param == 1)		//selected
	    glColor4f(view->selectedNodeColor.R, view->selectedNodeColor.G,
		      view->selectedNodeColor.B,
		      view->selectedNodeColor.A);

	filled = 0;
    }

    glLineWidth(view->LineWidth);
    if (!filled)
	glBegin(GL_LINE_LOOP);
    else
	glBegin(GL_POLYGON);
    for (i = 0; i < 360; i = i + 1) {
	//convert degrees into radians
	float degInRad = (float) (i * DEG2RAD);
	glVertex3f((GLfloat) (x + cos(degInRad) * xradius),
		   (GLfloat) (y + sin(degInRad) * yradius),globalz);
    }
    glEnd();
}

static void DrawPolygon(xdot_op * op, int param)
//void DrawPolygon(xdot_point* xpoint,int count, int filled)
{
    int i = 0;
    int filled;
    //SelectPolygon((sdot_op *) op);
    set_options((sdot_op *) op, param);

    if (op->kind == xd_filled_polygon) {
	if (param == 0)
	    glColor4f(view->fillColor.R, view->fillColor.G,
		      view->fillColor.B, view->fillColor.A);
	if (param == 1)		//selected
	    glColor4f(view->selectedNodeColor.R, view->selectedNodeColor.G,
		      view->selectedNodeColor.B,
		      view->selectedNodeColor.A);

	filled = 1;
    } else {
	filled = 0;
	if (param == 0)
	    glColor4f(view->penColor.R, view->penColor.G, view->penColor.B,
		      view->penColor.A);
	if (param == 1)		//selected
	    glColor4f(view->selectedNodeColor.R, view->selectedNodeColor.G,
		      view->selectedNodeColor.B,
		      view->selectedNodeColor.A);

    }
    glLineWidth(view->LineWidth);
    if (!filled)
	glBegin(GL_LINE_STRIP);
    else
	glBegin(GL_POLYGON);
    for (i = 0; i < op->u.polygon.cnt; i = i + 1) {
	glVertex3f((GLfloat) op->u.polygon.pts[i].x - dx,
		   (GLfloat) op->u.polygon.pts[i].y - dy,
		   (GLfloat) op->u.polygon.pts[i].z+globalz);
    }
    glVertex3f((GLfloat) op->u.polygon.pts[0].x - dx, (GLfloat) op->u.polygon.pts[0].y - dy, (GLfloat) op->u.polygon.pts[0].z+globalz);	//close the polygon
    glEnd();
}

static void DrawPolyline(xdot_op * op, int param)
{
    int i = 0;
    if (param == 0)
	glColor4f(view->penColor.R, view->penColor.G, view->penColor.B,
		  view->penColor.A);
    if (param == 1)		//selected
	glColor4f(view->selectedNodeColor.R, view->selectedNodeColor.G,
		  view->selectedNodeColor.B, view->selectedNodeColor.A);
    //SelectPolyline((sdot_op *) op);
    set_options((sdot_op *) op, param);
    glLineWidth(view->LineWidth);
    glBegin(GL_LINE_STRIP);
    for (i = 0; i < op->u.polyline.cnt; i = i + 1) {
	glVertex3f((GLfloat) op->u.polyline.pts[i].x - dx,
		   (GLfloat) op->u.polyline.pts[i].y - dy,
		   (GLfloat) op->u.polyline.pts[i].z+globalz);
    }
    glEnd();
}

#if 0
static void
DrawBitmap(GLfloat bmpX, GLfloat bmpY, GLfloat bmpW, GLfloat bmpH)
{
    if (view->texture)
	glEnable(GL_TEXTURE_2D);
    else
	glDisable(GL_TEXTURE_2D);
/*	glRasterPos2d(bmpX,bmpY);
	  glBitmap( bmpW,bmpH,0.0,0.0,0.0,0.0, 1); */
    glBegin(GL_QUADS);
    glTexCoord2d(bmpX, bmpY);
    glVertex2d(bmpX, bmpY);
    glTexCoord2d(bmpX + bmpW, bmpY);
    glVertex2d(bmpX + bmpW, bmpY);
    glTexCoord2d(bmpX + bmpW, bmpY + bmpH);
    glVertex2d(bmpX + bmpW, bmpY + bmpH);
    glTexCoord2d(bmpX, bmpY + bmpH);
    glVertex2d(bmpX, bmpY + bmpH);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}
#endif

static void SetFillColor(xdot_op * op, int param)
{
    RGBColor c;
    c = GetRGBColor(op->u.color);
    view->fillColor.R = c.R;
    view->fillColor.G = c.G;
    view->fillColor.B = c.B;
    view->fillColor.A = c.A;
}
static void SetPenColor(xdot_op * op, int param)
{
    RGBColor c;
    c = GetRGBColor(op->u.color);
    view->penColor.R = c.R;
    view->penColor.G = c.G;
    view->penColor.B = c.B;
    view->penColor.A = c.A;
}

static void SetStyle(xdot_op * op, int param)
{


}

static void SetFont(xdot_op * op, int param)
{
	//activate the right font
/*	view->widgets->fontset->activefont=add_font(view->widgets->fontset,op->u.font.name);//load or set active font
	view->FontSize =  op->u.font.size;*/
}

static void InsertImage(xdot_op * op, int param)
{
 //   SelectImage((sdot_op *) op);

}
static void EmbedText(xdot_op * op, int param)
{
    GLfloat x;
//    SelectText((sdot_op *) op);
    set_options((sdot_op *) op, param);
    if (op->u.text.align == 1)
	x = (GLfloat) op->u.text.x - (GLfloat) (op->u.text.width / 2.0);
    if (op->u.text.align == 0)
	x = (GLfloat) op->u.text.x;
    if (op->u.text.align == -1)
	x = (GLfloat) op->u.text.x + op->u.text.width;
	view->widgets->fontset->fonts[view->widgets->fontset->activefont]->fontheight=view->FontSize;
    if (param == 0)
	fontColor(view->widgets->fontset->fonts[view->widgets->fontset->activefont],view->penColor.R, view->penColor.G, view->penColor.B,1);
    if (param == 1)		//selected
	fontColor(view->widgets->fontset->fonts[view->widgets->fontset->activefont],view->selectedNodeColor.R, view->selectedNodeColor.G,
		  view->selectedNodeColor.B,1);
	glprintf(view->widgets->fontset->fonts[view->widgets->fontset->activefont], (x - dx), (GLfloat)op->u.text.y -  dy,
		   (GLfloat)op->u.text.width, op->u.text.text);
}

void draw_selection_box(ViewInfo * view)
{
    if (((view->mouse.mouse_mode == 4) || (view->mouse.mouse_mode == 5))
	&& view->mouse.mouse_down) {
	glColor4f(view->Selection.SelectionColor.R,
		  view->Selection.SelectionColor.G,
		  view->Selection.SelectionColor.B,
		  view->Selection.SelectionColor.A);
	if (view->mouse.mouse_mode == 5) {
	    glEnable(GL_LINE_STIPPLE);
	    glLineStipple(1, 15);
	}
	glBegin(GL_LINE_STRIP);
	glVertex3f((GLfloat) view->GLx, (GLfloat) view->GLy,
		   (GLfloat) 0.001+globalz);
	glVertex3f((GLfloat) view->GLx, (GLfloat) view->GLy2,
		   (GLfloat) 0.001+globalz);
	glVertex3f((GLfloat) view->GLx2, (GLfloat) view->GLy2,
		   (GLfloat) 0.001+globalz);
	glVertex3f((GLfloat) view->GLx2, (GLfloat) view->GLy,
		   (GLfloat) 0.001+globalz);
	glVertex3f((GLfloat) view->GLx, (GLfloat) view->GLy,
		   (GLfloat) 0.001+globalz);
	glEnd();
	if (view->mouse.mouse_mode == 5)
	    glDisable(GL_LINE_STIPPLE);

    }
}

void draw_magnifier(ViewInfo * view)
{

    if ((view->mouse.mouse_mode == MM_MAGNIFIER)
	&& (view->mouse.mouse_down)) {

	GLfloat mg_x, mg_y, mg_z;
	//converting screen pixel distaances to GL distances
	view->mg.GLwidth = GetOGLDistance(view->mg.width) / (float) 2.0;
	view->mg.GLheight = GetOGLDistance(view->mg.height) / (float) 2.0;
	GetOGLPosRef((int)view->mouse.mouse_X, (int)view->mouse.mouse_Y, &mg_x, &mg_y, &mg_z);	//retrieving mouse coords as GL coordinates
	view->mg.x = mg_x;
	view->mg.y = mg_y;
	glLineWidth(4);
	local_zoom(view->Topview);
	//drawing the magnifier borders
	glBegin(GL_LINE_STRIP);
	glColor4f((GLfloat) 0.3, (GLfloat) 0.1, (GLfloat) 0.8,
		  (GLfloat) 1);
	glVertex3f(view->mg.x - view->mg.GLwidth,
		   view->mg.y - view->mg.GLheight, Z_MIDDLE_PLANE);
	glVertex3f(view->mg.x + view->mg.GLwidth,
		   view->mg.y - view->mg.GLheight, Z_MIDDLE_PLANE);
	glVertex3f(view->mg.x + view->mg.GLwidth,
		   view->mg.y + view->mg.GLheight, Z_MIDDLE_PLANE);
	glVertex3f(view->mg.x - view->mg.GLwidth,
		   view->mg.y + view->mg.GLheight, Z_MIDDLE_PLANE);
	glVertex3f(view->mg.x - view->mg.GLwidth,
		   view->mg.y - view->mg.GLheight, Z_MIDDLE_PLANE);
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1, 1, 1, 1);
	glVertex3f(view->mg.x - view->mg.GLwidth + 1,
		   view->mg.y - view->mg.GLheight + 1, Z_MIDDLE_PLANE);
	glVertex3f(view->mg.x + view->mg.GLwidth - 1,
		   view->mg.y - view->mg.GLheight + 1, Z_MIDDLE_PLANE);
	glVertex3f(view->mg.x + view->mg.GLwidth - 1,
		   view->mg.y + view->mg.GLheight - 1, Z_MIDDLE_PLANE);
	glVertex3f(view->mg.x - view->mg.GLwidth + 1,
		   view->mg.y + view->mg.GLheight - 1, Z_MIDDLE_PLANE);
	glVertex3f(view->mg.x - view->mg.GLwidth + 1,
		   view->mg.y - view->mg.GLheight + 1, Z_MIDDLE_PLANE);
	glEnd();
	glLineWidth(1);
    }

}

static void draw_circle(float originX, float originY, float radius)
{
/* draw a circle from a bunch of short lines */
    float vectorX1, vectorY1, vectorX, vectorY, angle;
    vectorY1 = originY + radius;
    vectorX1 = originX;
    glLineWidth(4);
    glBegin(GL_LINE_STRIP);
    for (angle = (float) 0.0; angle <= (float) (2.1 * 3.14159);
	 angle += (float) 0.1) {
	vectorX = originX + radius * (float) sin(angle);
	vectorY = originY + radius * (float) cos(angle);
	glVertex3d(vectorX1, vectorY1,globalz);
	vectorY1 = vectorY;
	vectorX1 = vectorX;
    }
    glEnd();
    glLineWidth(1);

}

static GLUquadric* fisheyesphere;
void draw_fisheye_magnifier(ViewInfo * view)
{
    if ((view->mouse.mouse_mode == 21) && (view->mouse.mouse_down)) {
	float a;
	GLfloat mg_x, mg_y, mg_z;
	a = GetOGLDistance((int) view->fmg.constantR);
	view->fmg.R = (int) a;
	GetOGLPosRef((int)view->mouse.mouse_X, (int)view->mouse.mouse_Y, &mg_x,
		     &mg_y, &mg_z);
	glColor4f((GLfloat) 0.3, (GLfloat) 0.1, (GLfloat) 0.8,
		  (GLfloat) 1);
	if (((view->fmg.x != mg_x) || (view->fmg.y != mg_y))&&(view->active_camera==-1)) {
		fisheye_polar(mg_x, mg_y, view->Topview);
	    draw_circle(mg_x, mg_y, a);
	}
	if (((view->fmg.x != mg_x) || (view->fmg.y != mg_y))&&(view->active_camera>-1)) {
		fisheye_spherical(mg_x, mg_y,0.00,view->Topview);


		if(!fisheyesphere)
			fisheyesphere=gluNewQuadric();
		gluQuadricDrawStyle ( fisheyesphere, GLU_LINE );
		glColor4f((GLfloat) 0.3, (GLfloat) 0.1, (GLfloat) 0.8,(GLfloat)0.05);
		glTranslatef(mg_x,mg_y,0);
		gluSphere(fisheyesphere,a,30,30);
		glTranslatef(-mg_x,-mg_y,0);
	}


	view->fmg.x = mg_x;
	view->fmg.y = mg_y;

    }
}

void drawBorders(ViewInfo * view)
{
    if (view->bdVisible) {
		glColor4f(view->borderColor.R,view->borderColor.B,view->borderColor.G,view->borderColor.A);
	glLineWidth(2);
	glBegin(GL_LINE_STRIP);
	glVertex2d(view->bdxLeft, view->bdyBottom);
	glVertex2d(view->bdxRight, view->bdyBottom);
	glVertex2d(view->bdxRight, view->bdyTop);
	glVertex2d(view->bdxLeft, view->bdyTop);
	glVertex2d(view->bdxLeft, view->bdyBottom);
	glEnd();
	glLineWidth(1);
    }
}

/*static void drawXdot(xdot * xDot, int param, void *p)
{
    int id;
    sdot_op *ops = (sdot_op *) (xDot->ops);
    sdot_op *op;
	//to avoid the overlapping , z is slightly increased for each xdot of a particular object
	if (AGTYPE(p)==AGEDGE)
		globalz=1;	
	else
		globalz=0;	

	for (id = 0; id < xDot->cnt; id++)
	{
		globalz +=  (float)GLOBAL_Z_OFFSET;
		op = ops + id;
		op->obj = p;
		op->op.drawfunc(&(op->op), param);
    }
    if (OD_Preselected(p) == 1)
		select_object(view->g[view->activeGraph], p);
    OD_Preselected(p) = 0;
}*/


#ifdef UNUSED
static void drawXdotwithattr(void *p, char *attr, int param)
{
/*    xdot *xDot;
    if ((xDot = parseXDotF(agget(p, attr), OpFns, sizeof(sdot_op))))
	{
		drawXdot(xDot, param, p);
		freeXDot(xDot);
    }*/
}

static void drawXdotwithattrs(void *e, int param)
{
 /*   drawXdotwithattr(e, "_draw_", param);
    drawXdotwithattr(e, "_ldraw_", param);
    drawXdotwithattr(e, "_hdraw_", param);
    drawXdotwithattr(e, "_tdraw_", param);
    drawXdotwithattr(e, "_hldraw_", param);
    drawXdotwithattr(e, "_tldraw_", param);*/
}
#endif



/*void drawGraph(Agraph_t * g)
{
    Agnode_t *v;
    Agedge_t *e;
    Agraph_t *s;
    int param = 0;
	for (s = agfstsubg(g); s; s = agnxtsubg(s))
	{
		OD_SelFlag(s) = 0;
		if (OD_Selected(s) == 1)
			param = 1;
		else
		    param = 0;
		drawXdotwithattrs(s, param);
    }

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
	{
		if (OD_Selected(v) == 1)
			param = 1;
		else
			param = 0;
		OD_SelFlag(v) = 0;
		drawXdotwithattr(v, "_draw_", param); //draw primitives
		drawXdotwithattr(v, "_ldraw_", param);//label drawing
		for (e = agfstout(g, v); e; e = agnxtout(g, e)) 
		{
			OD_SelFlag(e) = 0;
			if (OD_Selected(e) == 1)
				param = 1;
			else
				param = 0;
		    drawXdotwithattrs(e, param);
		}
    }
    if ((view->Selection.Active > 0) && (!view->SignalBlock)) 
	{
		view->Selection.Active = 0;
		drawGraph(g);
		view->SignalBlock = 1;
		glexpose();
		view->SignalBlock = 0;
    }

}*/


/*
	this function is used to cache fonts in view->fontset
*/

static void scanXdot(xdot * xDot, void *p)
{
    int id;
    sdot_op *ops = (sdot_op *) (xDot->ops);
    sdot_op *op;

    for (id = 0; id < xDot->cnt; id++)
	{
		op = ops + id;
		op->obj = p;
		if (op->op.kind==xd_font)
		{
			add_font(view->widgets->fontset,op->op.u.font.name);//load or set active font
		}
    }

}


static void scanXdotwithattr(void *p, char *attr)
{
    xdot *xDot;
    if ((xDot = parseXDotF(agget(p, attr), OpFns, sizeof(sdot_op))))
	{
		scanXdot(xDot, p);
		freeXDot(xDot);
    }
}

static void scanXdotwithattrs(void *e)
{
    scanXdotwithattr(e, "_draw_");
    scanXdotwithattr(e, "_ldraw_");
    scanXdotwithattr(e, "_hdraw_");
    scanXdotwithattr(e, "_tdraw_");
    scanXdotwithattr(e, "_hldraw_");
    scanXdotwithattr(e, "_tldraw_");
}



/*
	iterate in nodes and edges to cache fonts, run this once or whenever a new font is added to the graph
*/


void scanGraph(Agraph_t * g)
{
    Agnode_t *v;
    Agedge_t *e;
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
	{
		scanXdotwithattr(v, "_draw_"); 
		scanXdotwithattr(v, "_ldraw_");
		for (e = agfstout(g, v); e; e = agnxtout(g, e)) 
		{
	      scanXdotwithattrs(e);
		}
    }

}
int randomize_color(RGBColor * c, int brightness)
{
    float R, B, G;
    float add;
    R = (float) (rand() % 255) / (float) 255.0;
    G = (float) (rand() % 255) / (float) 255.0;
    B = (float) (rand() % 255) / (float) 255.0;
    add = (brightness - (R + G + B)) / 3;
    R = R;
    G = G;
    B = B;
    c->R = R;
    c->G = G;
    c->B = B;
    return 1;
}


void drawCircle(float x, float y, float radius, float zdepth)
{
    int i;
    if (radius < 0.3)
	radius = (float) 0.4;
    glBegin(GL_POLYGON);
    for (i = 0; i < 360; i = i + 10) {
	float degInRad = (float) (i * DEG2RAD);
	glVertex3f((GLfloat) (x + cos(degInRad) * radius),
		   (GLfloat) (y + sin(degInRad) * radius),
		   (GLfloat) zdepth+globalz);
    }

    glEnd();
}

drawfunc_t OpFns[] = {
    DrawEllipse,
    DrawPolygon,
    DrawBeziers,
    DrawPolyline,
    EmbedText,
    SetFillColor,
    SetPenColor,
    SetFont,
    SetStyle,
    InsertImage,
};

RGBColor GetRGBColor(char *color)
{
    gvcolor_t cl;
    RGBColor c;
    if (color != '\0') {

	colorxlate(color, &cl, RGBA_DOUBLE);
	c.R = (float) cl.u.RGBA[0];
	c.G = (float) cl.u.RGBA[1];
	c.B = (float) cl.u.RGBA[2];
	c.A = (float) cl.u.RGBA[3];
    } else {
	c.R = view->penColor.R;
	c.G = view->penColor.G;
	c.B = view->penColor.B;
	c.A = view->penColor.A;
    }
    return c;
}
void drawEllipse(float xradius, float yradius,int angle1,int angle2)
{
	int i;
	glBegin(GL_LINE_STRIP);

   for (i=angle1; i<=angle2; i++)
   {
      //convert degrees into radians
      float degInRad = (float)i*(float)DEG2RAD;
      glVertex3f((GLfloat)(cos(degInRad)*xradius),(GLfloat)(sin(degInRad)*yradius),globalz);
   }
 
   glEnd();
}
int draw_node_hintbox_gl_polygon(GLfloat x,GLfloat y,GLfloat z,GLfloat fs,char* text)

{


	GLfloat X,Y,Z,pad;
	pad=fs/(GLfloat)5.0;
	X=x;Y=y;Z=z+(GLfloat)0.0005;
	glBegin(GL_POLYGON);
	glVertex3f(X,Y,Z);
	Y=Y + fs;
	glVertex3f(X,Y,Z);
	X=X+fs;
	glVertex3f(X,Y,Z);
	X=x;y=y;
	glVertex3f(X,Y,Z);
	glEnd();

	X=x;Y=y+fs;
	glBegin(GL_POLYGON);
	glVertex3f(X,Y,Z);
	X=x;Y=Y+fs+2*pad;
	glVertex3f(X,Y,Z);
	X=x+strlen(text)*fs/(GLfloat)2.0+(GLfloat)2.0*pad;
	glVertex3f(X,Y,Z);
	Y=y+fs;
	glVertex3f(X,Y,Z);
	X=x;
	glVertex3f(X,Y,Z);
	glEnd();


	return 1;

}
int draw_node_hintbox_gl_line(GLfloat x,GLfloat y,GLfloat z,GLfloat fs,char* text)
{


	GLfloat X,Y,Z,pad;
	pad=fs/(GLfloat)5.0;
	X=x;Y=y;Z=z+(GLfloat)0.001;
	glBegin(GL_LINE_STRIP);
	glVertex3f(X,Y,Z);
	Y=Y + 2*fs+2*pad;
	glVertex3f(X,Y,Z);
	X=X+2*pad+strlen(text)*fs/(GLfloat)2.0;
	glVertex3f(X,Y,Z);
	Y=y+fs;
	glVertex3f(X,Y,Z);
	X=x+fs;
	glVertex3f(X,Y,Z);
	X=x;Y=y;
	glVertex3f(X,Y,Z);
	glEnd();



	return 1;

}


int draw_node_hintbox(GLfloat x,GLfloat y,GLfloat z,GLfloat fs,char* text)
{
	
	glColor3f(1,1,0);
		draw_node_hintbox_gl_polygon(x,y,z,fs,text);
	glColor3f(0,0,1);
		draw_node_hintbox_gl_line(x,y,z,fs,text);
	return 1;

}

static GLUquadric* sphere;
void draw_sphere(float x,float y,float z,float r)
{
	if(!sphere)
			fisheyesphere=gluNewQuadric();
	gluQuadricDrawStyle ( fisheyesphere, GLU_FILL );
	glTranslatef(x,y,z);
	gluSphere(fisheyesphere,r,SPHERE_SLICE_COUNT,SPHERE_SLICE_COUNT);
	glTranslatef(-x,-y,-z);
}


