void gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);
void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
          GLfloat centerx, GLfloat centery, GLfloat centerz,
          GLfloat upx, GLfloat upy, GLfloat upz);
int gluUnProject(GLfloat winx, GLfloat winy, GLfloat winz,
		  const GLfloat modelMatrix[16],
		  const GLfloat projMatrix[16],
		  const GLint viewport[4],
		  GLfloat * objx, GLfloat * objy, GLfloat * objz);
