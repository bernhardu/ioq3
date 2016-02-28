/*
 * Copyright (C) 2009  Nokia Corporation.  All rights reserved.
 */

#ifndef __ANDROID_GLIMP_H__
#define __ANDROID_GLIMP_H__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

#include <GLES/gl.h>
#include "../android/qgl.h"

#ifndef GLAPI
#define GLAPI extern
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifndef APIENTRY
#define APIENTRY GLAPIENTRY
#endif

/* "P" suffix to be used for a pointer to a function */
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#ifndef GLAPIENTRYP
#define GLAPIENTRYP GLAPIENTRY *
#endif

#define qglOrtho qglOrthof
#define qglDepthRange qglDepthRangef
#define qglClearDepth qglClearDepthf
#define qglColor4f glColor4f

#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1
#define GL_TEXTURE2_ARB                     0x84C2
#define GL_TEXTURE3_ARB                     0x84C3

#define GL_RGB_S3TC                         0x83A0
#define GL_RGB4_S3TC                        0x83A1

typedef double GLdouble;

#ifndef GL_ARB_vertex_buffer_object
/* GL types for handling large vertex buffer objects */
#if defined(__APPLE__)
typedef long GLintptrARB;
typedef long GLsizeiptrARB;
#else
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
#endif
#endif

#ifndef GL_ARB_vertex_buffer_object
/* GL types for handling large vertex buffer objects */
#if defined(__APPLE__)
typedef long GLintptrARB;
typedef long GLsizeiptrARB;
#else
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
#endif
#endif

#ifndef GL_ARB_shader_objects
/* GL types for program/shader text and shader object handles */
typedef char GLcharARB;
#if defined(__APPLE__)
typedef void *GLhandleARB;
#else
typedef unsigned int GLhandleARB;
#endif
#endif

void GLimp_Init(void);
void GLimp_LogComment(char *comment);
void GLimp_EndFrame(void);
void GLimp_Shutdown(void);
void GLimp_Minimize(void);
void qglArrayElement(GLint i);
void qglCallList(GLuint list);
void qglDrawBuffer(GLenum mode);
void qglLockArrays(GLint i, GLsizei size);
void qglUnlockArrays(void);
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256],
		    unsigned char blue[256]);

static inline void qglClipPlane(GLenum plane, const GLdouble *equation)
{
    float plane2[4];
    plane2[0] = equation[0];
    plane2[1] = equation[1];
    plane2[2] = equation[2];
    plane2[3] = equation[3];
    qglClipPlanef(plane, plane2);
}

static inline void qglColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
    glColor4f(red, green, blue, 1.0f);
}

#endif
