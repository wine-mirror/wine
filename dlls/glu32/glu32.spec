@ stdcall gluLookAt(double double double double double double double double double) wine_gluLookAt
@ stdcall gluOrtho2D(double double double double) wine_gluOrtho2D
@ stdcall gluPerspective(double double double double) wine_gluPerspective
@ stdcall gluPickMatrix(double double double double ptr) wine_gluPickMatrix
@ stdcall gluProject(double double double ptr ptr ptr ptr ptr ptr) wine_gluProject
@ stdcall gluUnProject(double double double ptr ptr ptr ptr ptr ptr) wine_gluUnProject
@ stdcall gluErrorString(long) wine_gluErrorString
@ stub gluErrorUnicodeStringEXT
@ stdcall gluScaleImage(long long long long ptr long long long ptr) wine_gluScaleImage
@ stdcall gluBuild1DMipmaps(long long long long long ptr) wine_gluBuild1DMipmaps
@ stdcall gluBuild2DMipmaps(long long long long long long ptr) wine_gluBuild2DMipmaps
@ stdcall gluNewQuadric() wine_gluNewQuadric
@ stdcall gluDeleteQuadric(ptr) wine_gluDeleteQuadric
@ stdcall gluQuadricDrawStyle(ptr long) wine_gluQuadricDrawStyle
@ stdcall gluQuadricOrientation(ptr long) wine_gluQuadricOrientation
@ stdcall gluQuadricNormals(ptr long) wine_gluQuadricNormals
@ stdcall gluQuadricTexture(ptr long) wine_gluQuadricTexture
@ stdcall gluQuadricCallback(ptr long ptr) wine_gluQuadricCallback
@ stdcall gluCylinder(ptr double double double long long) wine_gluCylinder
@ stdcall gluSphere(ptr double long long) wine_gluSphere
@ stdcall gluDisk(ptr double double long long) wine_gluDisk
@ stdcall gluPartialDisk(ptr double double long long double double) wine_gluPartialDisk
@ stdcall gluNewNurbsRenderer() wine_gluNewNurbsRenderer
@ stdcall gluDeleteNurbsRenderer(ptr) wine_gluDeleteNurbsRenderer
@ stdcall gluLoadSamplingMatrices(ptr ptr ptr ptr) wine_gluLoadSamplingMatrices
@ stdcall gluNurbsProperty(ptr long long) wine_gluNurbsProperty
@ stdcall gluGetNurbsProperty(ptr long ptr) wine_gluGetNurbsProperty
@ stdcall gluBeginCurve(ptr) wine_gluBeginCurve
@ stdcall gluEndCurve(ptr) wine_gluEndCurve
@ stdcall gluNurbsCurve(ptr long ptr long ptr long long) wine_gluNurbsCurve
@ stdcall gluBeginSurface(ptr) wine_gluBeginSurface
@ stdcall gluEndSurface(ptr) wine_gluEndSurface
@ stdcall gluNurbsSurface(ptr long ptr long ptr long long ptr long long long) wine_gluNurbsSurface
@ stdcall gluBeginTrim(ptr) wine_gluBeginTrim
@ stdcall gluEndTrim(ptr) wine_gluEndTrim
@ stdcall gluPwlCurve(ptr long ptr long long) wine_gluPwlCurve
@ stdcall gluNurbsCallback(ptr long ptr) wine_gluNurbsCallback
@ stdcall gluNewTess() wine_gluNewTess
@ stdcall gluDeleteTess(ptr) wine_gluDeleteTess
@ stub gluGetTessProperty
@ stub gluTessBeginContour
@ stub gluTessBeginPolygon
@ stub gluTessEndContour
@ stub gluTessEndPolygon
@ stub gluTessNormal
@ stub gluTessProperty
@ stdcall gluTessVertex(ptr ptr ptr) wine_gluTessVertex
@ stdcall gluTessCallback(ptr long ptr) wine_gluTessCallback
@ stdcall gluBeginPolygon(ptr) wine_gluBeginPolygon
@ stdcall gluEndPolygon(ptr) wine_gluEndPolygon
@ stdcall gluNextContour(ptr long) wine_gluNextContour
@ stdcall gluGetString(long) wine_gluGetString
@ stdcall gluCheckExtension(str ptr) wine_gluCheckExtension
