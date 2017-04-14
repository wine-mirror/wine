@ stdcall gluBeginCurve(ptr) wine_gluBeginCurve
@ stdcall gluBeginPolygon(ptr) wine_gluBeginPolygon
@ stdcall gluBeginSurface(ptr) wine_gluBeginSurface
@ stdcall gluBeginTrim(ptr) wine_gluBeginTrim
@ stdcall gluBuild1DMipmaps(long long long long long ptr) wine_gluBuild1DMipmaps
@ stdcall gluBuild2DMipmaps(long long long long long long ptr) wine_gluBuild2DMipmaps
@ stdcall gluCheckExtension(str ptr) wine_gluCheckExtension
@ stdcall gluCylinder(ptr double double double long long)
@ stdcall gluDeleteNurbsRenderer(ptr) wine_gluDeleteNurbsRenderer
@ stdcall gluDeleteQuadric(ptr)
@ stdcall gluDeleteTess(ptr) wine_gluDeleteTess
@ stdcall gluDisk(ptr double double long long)
@ stdcall gluEndCurve(ptr) wine_gluEndCurve
@ stdcall gluEndPolygon(ptr) wine_gluEndPolygon
@ stdcall gluEndSurface(ptr) wine_gluEndSurface
@ stdcall gluEndTrim(ptr) wine_gluEndTrim
@ stdcall gluErrorString(long) wine_gluErrorString
@ stdcall gluErrorUnicodeStringEXT(long) wine_gluErrorUnicodeStringEXT
@ stdcall gluGetNurbsProperty(ptr long ptr) wine_gluGetNurbsProperty
@ stdcall gluGetString(long) wine_gluGetString
@ stdcall gluGetTessProperty(ptr long ptr) wine_gluGetTessProperty
@ stdcall gluLoadSamplingMatrices(ptr ptr ptr ptr) wine_gluLoadSamplingMatrices
@ stdcall gluLookAt(double double double double double double double double double)
@ stdcall gluNewNurbsRenderer() wine_gluNewNurbsRenderer
@ stdcall gluNewQuadric()
@ stdcall gluNewTess() wine_gluNewTess
@ stdcall gluNextContour(ptr long) wine_gluNextContour
@ stdcall gluNurbsCallback(ptr long ptr) wine_gluNurbsCallback
@ stdcall gluNurbsCurve(ptr long ptr long ptr long long) wine_gluNurbsCurve
@ stdcall gluNurbsProperty(ptr long long) wine_gluNurbsProperty
@ stdcall gluNurbsSurface(ptr long ptr long ptr long long ptr long long long) wine_gluNurbsSurface
@ stdcall gluOrtho2D(double double double double)
@ stdcall gluPartialDisk(ptr double double long long double double)
@ stdcall gluPerspective(double double double double)
@ stdcall gluPickMatrix(double double double double ptr)
@ stdcall gluProject(double double double ptr ptr ptr ptr ptr ptr)
@ stdcall gluPwlCurve(ptr long ptr long long) wine_gluPwlCurve
@ stdcall gluQuadricCallback(ptr long ptr)
@ stdcall gluQuadricDrawStyle(ptr long)
@ stdcall gluQuadricNormals(ptr long)
@ stdcall gluQuadricOrientation(ptr long)
@ stdcall gluQuadricTexture(ptr long)
@ stdcall gluScaleImage(long long long long ptr long long long ptr) wine_gluScaleImage
@ stdcall gluSphere(ptr double long long)
@ stdcall gluTessBeginContour(ptr) wine_gluTessBeginContour
@ stdcall gluTessBeginPolygon(ptr ptr) wine_gluTessBeginPolygon
@ stdcall gluTessCallback(ptr long ptr) wine_gluTessCallback
@ stdcall gluTessEndContour(ptr) wine_gluTessEndContour
@ stdcall gluTessEndPolygon(ptr) wine_gluTessEndPolygon
@ stdcall gluTessNormal(ptr double double double) wine_gluTessNormal
@ stdcall gluTessProperty(ptr long double) wine_gluTessProperty
@ stdcall gluTessVertex(ptr ptr ptr) wine_gluTessVertex
@ stdcall gluUnProject(double double double ptr ptr ptr ptr ptr ptr)
