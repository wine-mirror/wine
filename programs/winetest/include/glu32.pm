package glu32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "gluBeginCurve" => ["long",  ["ptr"]],
    "gluBeginPolygon" => ["long",  ["ptr"]],
    "gluBeginSurface" => ["long",  ["ptr"]],
    "gluBeginTrim" => ["long",  ["ptr"]],
    "gluBuild1DMipmaps" => ["long",  ["long", "long", "long", "long", "long", "ptr"]],
    "gluBuild2DMipmaps" => ["long",  ["long", "long", "long", "long", "long", "long", "ptr"]],
    "gluCheckExtension" => ["long",  ["ptr", "ptr"]],
    "gluCylinder" => ["long",  ["ptr", "double", "double", "double", "long", "long"]],
    "gluDeleteNurbsRenderer" => ["long",  ["ptr"]],
    "gluDeleteQuadric" => ["long",  ["ptr"]],
    "gluDeleteTess" => ["long",  ["ptr"]],
    "gluDisk" => ["long",  ["ptr", "double", "double", "long", "long"]],
    "gluEndCurve" => ["long",  ["ptr"]],
    "gluEndPolygon" => ["long",  ["ptr"]],
    "gluEndSurface" => ["long",  ["ptr"]],
    "gluEndTrim" => ["long",  ["ptr"]],
    "gluErrorString" => ["long",  ["long"]],
    "gluGetNurbsProperty" => ["long",  ["ptr", "long", "ptr"]],
    "gluGetString" => ["long",  ["long"]],
    "gluLoadSamplingMatrices" => ["long",  ["ptr", "ptr", "ptr", "ptr"]],
    "gluLookAt" => ["long",  ["double", "double", "double", "double", "double", "double", "double", "double", "double"]],
    "gluNewNurbsRenderer" => ["long",  ["undef"]],
    "gluNewQuadric" => ["long",  ["undef"]],
    "gluNewTess" => ["long",  ["undef"]],
    "gluNextContour" => ["long",  ["ptr", "long"]],
    "gluNurbsCallback" => ["long",  ["ptr", "long", "ptr"]],
    "gluNurbsCurve" => ["long",  ["ptr", "long", "ptr", "long", "ptr", "long", "long"]],
    "gluNurbsProperty" => ["long",  ["ptr", "long", "long"]],
    "gluNurbsSurface" => ["long",  ["ptr", "long", "ptr", "long", "ptr", "long", "long", "ptr", "long", "long", "long"]],
    "gluOrtho2D" => ["long",  ["double", "double", "double", "double"]],
    "gluPartialDisk" => ["long",  ["ptr", "double", "double", "long", "long", "double", "double"]],
    "gluPerspective" => ["long",  ["double", "double", "double", "double"]],
    "gluPickMatrix" => ["long",  ["double", "double", "double", "double", "ptr"]],
    "gluProject" => ["long",  ["double", "double", "double", "ptr", "ptr", "ptr", "ptr", "ptr", "ptr"]],
    "gluPwlCurve" => ["long",  ["ptr", "long", "ptr", "long", "long"]],
    "gluQuadricCallback" => ["long",  ["ptr", "long", "ptr"]],
    "gluQuadricDrawStyle" => ["long",  ["ptr", "long"]],
    "gluQuadricNormals" => ["long",  ["ptr", "long"]],
    "gluQuadricOrientation" => ["long",  ["ptr", "long"]],
    "gluQuadricTexture" => ["long",  ["ptr", "long"]],
    "gluScaleImage" => ["long",  ["long", "long", "long", "long", "ptr", "long", "long", "long", "ptr"]],
    "gluSphere" => ["long",  ["ptr", "double", "long", "long"]],
    "gluTessCallback" => ["long",  ["ptr", "long", "ptr"]],
    "gluTessVertex" => ["long",  ["ptr", "ptr", "ptr"]],
    "gluUnProject" => ["long",  ["double", "double", "double", "ptr", "ptr", "ptr", "ptr", "ptr", "ptr"]]
};

&wine::declare("glu32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
