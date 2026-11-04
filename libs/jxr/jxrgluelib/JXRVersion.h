/*
 * File         : JXRVersion.h
 * Author       : Mirco Miranda
 * Created on   : 2026/5/22
 */
#ifndef JXRVERSION_H
#define JXRVERSION_H

#define JXR_VERSION_MAJOR 1
#define JXR_VERSION_MINOR 4
#define JXR_VERSION_PATCH 4

#define JXR_MAKEVERSION(major, minor, patch) (((major) << 16) | ((minor) << 8) | (patch))

#define JXR_VERSION JXR_MAKEVERSION(JXR_VERSION_MAJOR, JXR_VERSION_MINOR, JXR_VERSION_PATCH)

#endif // JXRVERSION_H
