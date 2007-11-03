@ stdcall D3DXVec2Normalize(ptr ptr)
@ stdcall D3DXVec2Hermite(ptr ptr ptr ptr ptr long)
@ stdcall D3DXVec2CatmullRom(ptr ptr ptr ptr long long)
@ stdcall D3DXVec2BaryCentric(ptr ptr ptr ptr long long)
@ stdcall D3DXVec2Transform(ptr ptr ptr)
@ stdcall D3DXVec2TransformCoord(ptr ptr ptr)
@ stdcall D3DXVec2TransformNormal(ptr ptr ptr)
@ stdcall D3DXVec3Normalize(ptr ptr)
@ stdcall D3DXVec3Hermite(ptr ptr ptr ptr ptr long)
@ stdcall D3DXVec3CatmullRom(ptr ptr ptr ptr long long)
@ stdcall D3DXVec3BaryCentric(ptr ptr ptr ptr long long)
@ stdcall D3DXVec3Transform(ptr ptr ptr)
@ stdcall D3DXVec3TransformCoord(ptr ptr ptr)
@ stdcall D3DXVec3TransformNormal(ptr ptr ptr)
@ stub D3DXVec3Project
@ stub D3DXVec3Unproject
@ stdcall D3DXVec4Cross(ptr ptr ptr ptr)
@ stdcall D3DXVec4Normalize(ptr ptr)
@ stdcall D3DXVec4Hermite(ptr ptr ptr ptr ptr long)
@ stdcall D3DXVec4CatmullRom(ptr ptr ptr ptr long long)
@ stdcall D3DXVec4BaryCentric(ptr ptr ptr ptr long long)
@ stdcall D3DXVec4Transform(ptr ptr ptr)
@ stdcall D3DXMatrixfDeterminant(ptr)
@ stdcall D3DXMatrixMultiply(ptr ptr ptr)
@ stdcall D3DXMatrixTranspose(ptr ptr)
@ stub D3DXMatrixInverse
@ stdcall D3DXMatrixScaling(ptr long long long)
@ stdcall D3DXMatrixTranslation(ptr long long long)
@ stdcall D3DXMatrixRotationX(ptr long)
@ stdcall D3DXMatrixRotationY(ptr long)
@ stdcall D3DXMatrixRotationZ(ptr long)
@ stdcall D3DXMatrixRotationAxis(ptr ptr long)
@ stdcall D3DXMatrixRotationQuaternion(ptr ptr)
@ stdcall D3DXMatrixRotationYawPitchRoll(ptr long long long)
@ stub D3DXMatrixTransformation
@ stub D3DXMatrixAffineTransformation
@ stdcall D3DXMatrixLookAtRH(ptr ptr ptr ptr ptr)
@ stdcall D3DXMatrixLookAtLH(ptr ptr ptr ptr)
@ stub D3DXMatrixPerspectiveRH
@ stub D3DXMatrixPerspectiveLH
@ stub D3DXMatrixPerspectiveFovRH
@ stub D3DXMatrixPerspectiveFovLH
@ stub D3DXMatrixPerspectiveOffCenterRH
@ stub D3DXMatrixPerspectiveOffCenterLH
@ stub D3DXMatrixOrthoRH
@ stub D3DXMatrixOrthoLH
@ stub D3DXMatrixOrthoOffCenterRH
@ stub D3DXMatrixOrthoOffCenterLH
@ stub D3DXMatrixShadow
@ stub D3DXMatrixReflect
@ stub D3DXQuaternionToAxisAngle
@ stub D3DXQuaternionRotationMatrix
@ stub D3DXQuaternionRotationAxis
@ stub D3DXQuaternionRotationYawPitchRoll
@ stub D3DXQuaternionMultiply
@ stdcall D3DXQuaternionNormalize(ptr ptr)
@ stub D3DXQuaternionInverse
@ stub D3DXQuaternionLn
@ stub D3DXQuaternionExp
@ stub D3DXQuaternionSlerp
@ stub D3DXQuaternionSquad
@ stub D3DXQuaternionBaryCentric
@ stub D3DXPlaneNormalize
@ stub D3DXPlaneIntersectLine
@ stub D3DXPlaneFromPointNormal
@ stub D3DXPlaneFromPoints
@ stub D3DXPlaneTransform
@ stub D3DXColorAdjustSaturation
@ stub D3DXColorAdjustContrast
@ stub D3DXCreateMatrixStack
@ stdcall D3DXCreateFont(ptr ptr ptr)
@ stub D3DXCreateFontIndirect
@ stub D3DXCreateSprite
@ stub D3DXCreateRenderToSurface
@ stub D3DXCreateRenderToEnvMap
@ stdcall D3DXAssembleShaderFromFileA(ptr long ptr ptr ptr)
@ stdcall D3DXAssembleShaderFromFileW(ptr long ptr ptr ptr)
@ stdcall D3DXGetFVFVertexSize(long)
@ stub D3DXGetErrorStringA
@ stub D3DXGetErrorStringW
@ stdcall D3DXAssembleShader(ptr long long ptr ptr ptr)
@ stub D3DXCompileEffectFromFileA
@ stub D3DXCompileEffectFromFileW
@ stub D3DXCompileEffect
@ stub D3DXCreateEffect
@ stub D3DXCreateMesh
@ stub D3DXCreateMeshFVF
@ stub D3DXCreateSPMesh
@ stub D3DXCleanMesh
@ stub D3DXValidMesh
@ stub D3DXGeneratePMesh
@ stub D3DXSimplifyMesh
@ stub D3DXComputeBoundingSphere
@ stub D3DXComputeBoundingBox
@ stub D3DXComputeNormals
@ stdcall D3DXCreateBuffer(long ptr)
@ stub D3DXLoadMeshFromX
@ stub D3DXSaveMeshToX
@ stub D3DXCreatePMeshFromStream
@ stub D3DXCreateSkinMesh
@ stub D3DXCreateSkinMeshFVF
@ stub D3DXCreateSkinMeshFromMesh
@ stub D3DXLoadMeshFromXof
@ stub D3DXLoadSkinMeshFromXof
@ stub D3DXTesselateMesh
@ stub D3DXDeclaratorFromFVF
@ stub D3DXFVFFromDeclarator
@ stub D3DXWeldVertices
@ stub D3DXIntersect
@ stub D3DXSphereBoundProbe
@ stub D3DXBoxBoundProbe
@ stub D3DXCreatePolygon
@ stub D3DXCreateBox
@ stub D3DXCreateCylinder
@ stub D3DXCreateSphere
@ stub D3DXCreateTorus
@ stub D3DXCreateTeapot
@ stub D3DXCreateTextA
@ stub D3DXCreateTextW
@ stub D3DXLoadSurfaceFromFileA
@ stub D3DXLoadSurfaceFromFileW
@ stub D3DXLoadSurfaceFromResourceA
@ stub D3DXLoadSurfaceFromResourceW
@ stub D3DXLoadSurfaceFromFileInMemory
@ stub D3DXLoadSurfaceFromSurface
@ stub D3DXLoadSurfaceFromMemory
@ stub D3DXLoadVolumeFromVolume
@ stub D3DXLoadVolumeFromMemory
@ stub D3DXCheckTextureRequirements
@ stub D3DXCreateTexture
@ stub D3DXCreateTextureFromFileA
@ stub D3DXCreateTextureFromFileW
@ stub D3DXCreateTextureFromResourceA
@ stub D3DXCreateTextureFromResourceW
@ stub D3DXCreateTextureFromFileExA
@ stub D3DXCreateTextureFromFileExW
@ stub D3DXCreateTextureFromResourceExA
@ stub D3DXCreateTextureFromResourceExW
@ stub D3DXCreateTextureFromFileInMemory
@ stub D3DXCreateTextureFromFileInMemoryEx
@ stub D3DXFilterTexture
@ stub D3DXCheckCubeTextureRequirements
@ stub D3DXCreateCubeTexture
@ stub D3DXCreateCubeTextureFromFileA
@ stub D3DXCreateCubeTextureFromFileW
@ stub D3DXCreateCubeTextureFromFileExA
@ stub D3DXCreateCubeTextureFromFileExW
@ stub D3DXCreateCubeTextureFromFileInMemory
@ stub D3DXCreateCubeTextureFromFileInMemoryEx
@ stub D3DXFilterCubeTexture
@ stub D3DXCheckVolumeTextureRequirements
@ stub D3DXCreateVolumeTexture
@ stub D3DXFilterVolumeTexture
