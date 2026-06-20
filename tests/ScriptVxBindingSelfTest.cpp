#include "ScriptSelfTests.h"

#include <string>

#include "CKDefines.h"
#include "VxDefines.h"
#include "VxMath.h"

#include <angelscript.h>

#include "ScriptNativePointer.h"

static bool ExecuteVxBindingScriptSmoke(asIScriptEngine *engine, std::string &error) {
    if (!engine) {
        error = "Vx binding self-test requires an AngelScript engine.";
        return false;
    }

    constexpr const char *moduleName = "__CKAS_VxBindingSelfTest";
    const char *source =
        "float ReadConstFrustumRBound(const VxFrustum &in frustum) {\n"
        "  return frustum.GetRBound();\n"
        "}\n"
        "uint8 ReadConstXStringIndex(const XString &in value) {\n"
        "  return value[0];\n"
        "}\n"
        "VX_EFFECTCALLBACK_RETVAL VxEffectSmokeCallback(CKRenderContext@ dev, CKMaterial@ mat, int stage, NativePointer argument) {\n"
        "  if (dev !is null || mat !is null || stage != 3 || argument.IsNull()) return VXEFFECTRETVAL_SKIPALL;\n"
        "  return VXEFFECTRETVAL_SKIPNONE;\n"
        "}\n"
        "int Run() {\n"
        "  XString xstr(\"abaca\");\n"
        "  if (!(xstr < XString(\"abacb\"))) return 859;\n"
        "  if (!(xstr < \"abacb\")) return 860;\n"
        "  if (!xstr.Contains(XString(\"ba\"))) return 861;\n"
        "  if (xstr.RFind(97) != 2) return 862;\n"
        "  if (xstr.Find(97) != 0) return 863;\n"
        "  if (xstr[1] != 98) return 864;\n"
        "  xstr[1] = 90;\n"
        "  if (xstr[1] != 90 || ReadConstXStringIndex(xstr) != 97) return 865;\n"
        "  XString xcopy(xstr);\n"
        "  if (!(xcopy == xstr) || string(xcopy) != \"aZaca\") return 866;\n"
        "  CKPathSplitter pathSplitter(\"name.ext\");\n"
        "  if (pathSplitter.GetDrive() != \"\" || pathSplitter.GetDir() != \"\") return 870;\n"
        "  if (pathSplitter.GetName() != \"name\" || pathSplitter.GetExtension() != \".ext\") return 871;\n"
        "  CKPathMaker pathMaker(\"\", \"\", pathSplitter.GetName(), pathSplitter.GetExtension());\n"
        "  if (pathMaker.GetFileName() != \"name.ext\") return 872;\n"
        "  CKPOINT point;\n"
        "  point.x = 12;\n"
        "  point.y = 34;\n"
        "  if (point.x != 12 || point.y != 34) return 873;\n"
        "  CKPOINT copiedPoint(point);\n"
        "  if (copiedPoint.x != 12 || copiedPoint.y != 34) return 874;\n"
        "  CKPOINT assignedPoint;\n"
        "  assignedPoint = copiedPoint;\n"
        "  if (assignedPoint.x != 12 || assignedPoint.y != 34) return 875;\n"
        "  VxWindowFromPoint(point);\n"
        "  Vx2DCapsDesc caps2d;\n"
        "  caps2d.Family = CKRST_DIRECTX;\n"
        "  caps2d.MaxVideoMemory = 0x01020304;\n"
        "  caps2d.AvailableVideoMemory = 0x05060708;\n"
        "  caps2d.Caps = 0x11223344;\n"
        "  if (caps2d.Family != CKRST_DIRECTX || caps2d.MaxVideoMemory != 0x01020304) return 820;\n"
        "  if (caps2d.AvailableVideoMemory != 0x05060708 || caps2d.Caps != 0x11223344) return 821;\n"
        "  Vx2DCapsDesc copiedCaps2d(caps2d);\n"
        "  if (copiedCaps2d.AvailableVideoMemory != 0x05060708 || copiedCaps2d.Caps != 0x11223344) return 822;\n"
        "  Vx2DCapsDesc assignedCaps2d;\n"
        "  assignedCaps2d = copiedCaps2d;\n"
        "  if (assignedCaps2d.Family != CKRST_DIRECTX || assignedCaps2d.MaxVideoMemory != 0x01020304) return 823;\n"
        "  VxDriverDesc driverCaps2d;\n"
        "  driverCaps2d.Caps2D = assignedCaps2d;\n"
        "  if (driverCaps2d.Caps2D.AvailableVideoMemory != 0x05060708 || driverCaps2d.Caps2D.Caps != 0x11223344) return 824;\n"
        "  VxDisplayMode displayMode;\n"
        "  displayMode.Width = 1920;\n"
        "  displayMode.Height = 1080;\n"
        "  displayMode.Bpp = 32;\n"
        "  displayMode.RefreshRate = 60;\n"
        "  if (displayMode.Width != 1920 || displayMode.Height != 1080) return 830;\n"
        "  if (displayMode.Bpp != 32 || displayMode.RefreshRate != 60) return 831;\n"
        "  VxDisplayMode copiedDisplayMode(displayMode);\n"
        "  if (copiedDisplayMode.Width != 1920 || copiedDisplayMode.RefreshRate != 60) return 832;\n"
        "  VxDisplayMode assignedDisplayMode;\n"
        "  assignedDisplayMode = copiedDisplayMode;\n"
        "  if (assignedDisplayMode.Height != 1080 || assignedDisplayMode.Bpp != 32) return 833;\n"
#if CKVERSION == 0x13022002
        "  if (!(assignedDisplayMode == copiedDisplayMode)) return 834;\n"
        "  assignedDisplayMode.RefreshRate = 75;\n"
        "  if (!(assignedDisplayMode != copiedDisplayMode)) return 835;\n"
#endif
        "  Vx3DCapsDesc caps;\n"
        "  caps.CKRasterizerSpecificCaps = 0x12345678;\n"
        "  Vx3DCapsDesc copied(caps);\n"
        "  if (copied.CKRasterizerSpecificCaps != 0x12345678) return 1;\n"
        "  Vx3DCapsDesc assigned;\n"
        "  assigned = copied;\n"
        "  if (assigned.CKRasterizerSpecificCaps != 0x12345678) return 2;\n"
        "  VxColor color = {0.25f, 0.5f, 0.75f, 1.0f};\n"
        "  if (color.r != 0.25f || color.g != 0.5f || color.b != 0.75f || color.a != 1.0f) return 3;\n"
        "  VxColor other(0.25f, 0.5f, 0.75f, 1.0f);\n"
        "  if (color.GetSquareDistance(other) != 0.0f) return 4;\n"
        "  if (RGBAFTOCOLOR(color) != color.GetRGBA()) return 5;\n"
        "  VxImageDescEx imageDesc;\n"
        "  imageDesc.Width = 2;\n"
        "  imageDesc.Height = 2;\n"
        "  imageDesc.BytesPerLine = 8;\n"
        "  imageDesc.BitsPerPixel = 32;\n"
        "  imageDesc.RedMask = 0x00ff0000;\n"
        "  imageDesc.GreenMask = 0x0000ff00;\n"
        "  imageDesc.BlueMask = 0x000000ff;\n"
        "  imageDesc.AlphaMask = 0xff000000;\n"
        "  if (!imageDesc.HasAlpha()) return 700;\n"
        "  VxImageDescEx imageCopy(imageDesc);\n"
        "  if (!(imageDesc == imageCopy)) return 701;\n"
        "  VxImageDescEx imageAssigned;\n"
        "  imageAssigned = imageDesc;\n"
        "  if (!(imageAssigned == imageDesc)) return 702;\n"
        "  NativeBuffer@ imageStorage = NativeBuffer(16);\n"
        "  NativeBuffer@ colorMapStorage = NativeBuffer(16);\n"
        "  imageDesc.Image = imageStorage.ToPointer();\n"
        "  imageDesc.ColorMap = colorMapStorage.ToPointer();\n"
        "  if (imageDesc.Image.IsNull() || imageDesc.ColorMap.IsNull()) return 703;\n"
        "  imageDesc.ColorMapEntries = 4;\n"
        "  imageDesc.BytesPerColorEntry = 4;\n"
        "  if (!imageDesc.SetImageBuffer(imageStorage)) return 706;\n"
        "  if (!imageDesc.SetColorMapBuffer(colorMapStorage)) return 707;\n"
        "  NativeBuffer@ imageView = imageDesc.GetImageBuffer();\n"
        "  NativeBuffer@ colorMapView = imageDesc.GetColorMapBuffer();\n"
        "  if (imageView is null || imageView.Size() != 16) return 708;\n"
        "  if (colorMapView is null || colorMapView.Size() != 16) return 709;\n"
        "  NativeBuffer@ alphaValues = NativeBuffer(4);\n"
        "  alphaValues.Fill(0x7f, 4);\n"
        "  if (!VxDoAlphaBlit(imageDesc, alphaValues)) return 710;\n"
        "  NativeBuffer@ mip = NativeBuffer(4);\n"
        "  if (!VxGenerateMipMap(imageDesc, mip)) return 711;\n"
        "  VxImageDescEx imagePointerCopy(imageDesc);\n"
        "  if (imagePointerCopy.Image.ToUInt() != imageDesc.Image.ToUInt() || imagePointerCopy.ColorMap.ToUInt() != imageDesc.ColorMap.ToUInt()) return 704;\n"
        "  VxImageDescEx imagePointerAssigned;\n"
        "  imagePointerAssigned = imageDesc;\n"
        "  if (imagePointerAssigned.Image.ToUInt() != imageDesc.Image.ToUInt() || imagePointerAssigned.ColorMap.ToUInt() != imageDesc.ColorMap.ToUInt()) return 705;\n"
        "  VXFONTINFO fontInfo;\n"
        "  fontInfo.FaceName = \"CKAS Font\";\n"
        "  fontInfo.Height = 14;\n"
        "  fontInfo.Weight = 400;\n"
        "  fontInfo.Italic = 1;\n"
        "  fontInfo.Underline = 0;\n"
        "  if (fontInfo.FaceName != \"CKAS Font\" || fontInfo.Height != 14 || fontInfo.Weight != 400) return 847;\n"
        "  if (fontInfo.Italic != 1 || fontInfo.Underline != 0) return 848;\n"
        "  VXFONTINFO copiedFontInfo(fontInfo);\n"
        "  if (copiedFontInfo.FaceName != \"CKAS Font\" || copiedFontInfo.Height != 14) return 857;\n"
        "  VXFONTINFO assignedFontInfo;\n"
        "  assignedFontInfo = copiedFontInfo;\n"
        "  if (assignedFontInfo.Weight != 400 || assignedFontInfo.Italic != 1) return 858;\n"
        "  FONT_HANDLE smokeFont = VxCreateFont(\"Arial\", 12, 400, false, false);\n"
        "  if (smokeFont == 0) return 849;\n"
        "  VXFONTINFO queriedFontInfo;\n"
        "  if (!VxGetFontInfo(smokeFont, queriedFontInfo)) { VxDeleteFont(smokeFont); return 856; }\n"
        "  VxDeleteFont(smokeFont);\n"
        "  VxIntersectionDesc intersectionDesc;\n"
        "  intersectionDesc.IntersectionPoint = VxVector(1.0f, 2.0f, 3.0f);\n"
        "  intersectionDesc.IntersectionNormal = VxVector(0.0f, 1.0f, 0.0f);\n"
        "  intersectionDesc.TexU = 0.25f;\n"
        "  intersectionDesc.TexV = 0.75f;\n"
        "  intersectionDesc.Distance = 12.5f;\n"
        "  intersectionDesc.FaceIndex = 42;\n"
        "  if (intersectionDesc.Object !is null || intersectionDesc.IntersectionPoint.z != 3.0f) return 859;\n"
        "  if (intersectionDesc.IntersectionNormal.y != 1.0f || intersectionDesc.TexU != 0.25f || intersectionDesc.TexV != 0.75f) return 860;\n"
        "  VxIntersectionDesc copiedIntersectionDesc(intersectionDesc);\n"
        "  if (copiedIntersectionDesc.Distance != 12.5f || copiedIntersectionDesc.FaceIndex != 42) return 861;\n"
        "  VxIntersectionDesc assignedIntersectionDesc;\n"
        "  assignedIntersectionDesc = copiedIntersectionDesc;\n"
        "  if (assignedIntersectionDesc.IntersectionPoint.x != 1.0f || assignedIntersectionDesc.Object !is null) return 862;\n"
        "  VxDirectXData dxData;\n"
        "  dxData.DxVersion = 0x700;\n"
        "  VxDirectXData dxCopy(dxData);\n"
        "  if (dxCopy.DxVersion != 0x700) return 800;\n"
        "  VxDirectXData dxAssigned;\n"
        "  dxAssigned = dxCopy;\n"
        "  if (dxAssigned.DxVersion != 0x700) return 801;\n"
        "  if (!dxData.DDBackBuffer.IsNull() || !dxData.DDPrimaryBuffer.IsNull() || !dxData.DDZBuffer.IsNull()) return 802;\n"
        "  if (!dxData.DirectDraw.IsNull() || !dxData.Direct3D.IsNull() || !dxData.DDClipper.IsNull()) return 803;\n"
        "  if (!dxData.D3DDevice.IsNull() || !dxData.D3DViewport.IsNull()) return 804;\n"
        "  NativeBuffer@ effectArgStorage = NativeBuffer(4);\n"
        "  NativePointer effectArg = effectArgStorage.ToPointer();\n"
        "  CK_EFFECTCALLBACK@ effectCallback = @VxEffectSmokeCallback;\n"
        "  if (effectCallback(null, null, 3, effectArg) != VXEFFECTRETVAL_SKIPNONE) return 840;\n"
        "  VxEffectDescription effectDesc;\n"
        "  effectDesc.EffectIndex = VXEFFECT_NONE;\n"
        "  effectDesc.Summary = \"CKAS effect\";\n"
        "  effectDesc.Description = \"CKAS effect description\";\n"
        "  effectDesc.DescImage = \"effect.png\";\n"
        "  effectDesc.MaxTextureCount = 3;\n"
        "  effectDesc.NeededTextureCoordsCount = 2;\n"
        "  effectDesc.Tex1Description = \"diffuse\";\n"
        "  effectDesc.Tex2Description = \"normal\";\n"
        "  effectDesc.Tex3Description = \"height\";\n"
        "  effectDesc.ParameterType = CKGUID(0x12345678, 0x9abcdef0);\n"
        "  effectDesc.ParameterDescription = \"strength\";\n"
        "  effectDesc.ParameterDefaultValue = \"1.0\";\n"
        "  effectDesc.set_CallbackArg(NativePointer());\n"
        "  if (effectDesc.Summary != \"CKAS effect\" || effectDesc.MaxTextureCount != 3) return 841;\n"
        "  if (effectDesc.ParameterType.d1 != 0x12345678 || effectDesc.ParameterType.d2 != 0x9abcdef0) return 842;\n"
        "  if (effectDesc.get_SetCallback().IsNull() == false || !effectDesc.get_CallbackArg().IsNull()) return 843;\n"
        "  VxEffectDescription copiedEffectDesc(effectDesc);\n"
        "  if (copiedEffectDesc.Description != \"CKAS effect description\" || !copiedEffectDesc.get_CallbackArg().IsNull()) return 844;\n"
        "  VxEffectDescription assignedEffectDesc;\n"
        "  assignedEffectDesc = copiedEffectDesc;\n"
        "  if (assignedEffectDesc.Tex2Description != \"normal\" || assignedEffectDesc.ParameterDefaultValue != \"1.0\") return 845;\n"
        "  assignedEffectDesc.set_CallbackArg(NativePointer());\n"
        "  if (!assignedEffectDesc.get_CallbackArg().IsNull()) return 846;\n"
        "  VxStats stats;\n"
        "  stats.NbTrianglesDrawn = 1;\n"
        "  stats.NbPointsDrawn = 2;\n"
        "  stats.NbLinesDrawn = 3;\n"
        "  stats.NbVerticesProcessed = 4;\n"
        "  stats.NbObjectDrawn = 5;\n"
        "  stats.SmoothedFps = 60.0f;\n"
        "  stats.RenderStateCacheHit = 6;\n"
        "  stats.RenderStateCacheMiss = 7;\n"
        "  stats.DevicePreCallbacks = 1.5f;\n"
        "  stats.SceneTraversalTime = 2.5f;\n"
        "  stats.TransparentObjectsSortTime = 3.5f;\n"
        "  stats.ObjectsRenderTime = 4.5f;\n"
        "  stats.ObjectsCallbacksTime = 5.5f;\n"
        "  stats.SkinTime = 6.5f;\n"
        "  stats.SpriteTime = 7.5f;\n"
        "  stats.SpriteCallbacksTime = 8.5f;\n"
        "  stats.DevicePostCallbacks = 9.5f;\n"
        "  stats.BackToFrontTime = 10.5f;\n"
        "  if (stats.NbTrianglesDrawn != 1 || stats.NbObjectDrawn != 5 || stats.SmoothedFps != 60.0f) return 810;\n"
        "  if (stats.DevicePreCallbacks != 1.5f || stats.BackToFrontTime != 10.5f) return 811;\n"
        "  VxStats copiedStats(stats);\n"
        "  if (copiedStats.NbVerticesProcessed != 4 || copiedStats.SpriteCallbacksTime != 8.5f) return 812;\n"
        "  VxStats assignedStats;\n"
        "  assignedStats = copiedStats;\n"
        "  if (assignedStats.RenderStateCacheMiss != 7 || assignedStats.ObjectsRenderTime != 4.5f) return 813;\n"
        "  VxSpriteRenderOptions spriteOptions;\n"
        "  spriteOptions.ModulateColor = 0x10203040;\n"
        "  spriteOptions.Options = 0x5;\n"
        "  spriteOptions.AlphaTestFunc = VXCMP_LESSEQUAL;\n"
        "  spriteOptions.SrcBlendMode = VXBLEND_SRCALPHA;\n"
        "  spriteOptions.Options2 = 0x3;\n"
        "  spriteOptions.DstBlendMode = VXBLEND_INVSRCALPHA;\n"
        "  spriteOptions.AlphaRefValue = 123;\n"
        "  if (spriteOptions.ModulateColor != 0x10203040 || spriteOptions.Options != 0x5 || spriteOptions.AlphaTestFunc != VXCMP_LESSEQUAL) return 850;\n"
        "  if (spriteOptions.SrcBlendMode != VXBLEND_SRCALPHA || spriteOptions.Options2 != 0x3 || spriteOptions.DstBlendMode != VXBLEND_INVSRCALPHA) return 851;\n"
        "  if (spriteOptions.AlphaRefValue != 123) return 852;\n"
        "  VxSpriteRenderOptions copiedSpriteOptions(spriteOptions);\n"
        "  if (copiedSpriteOptions.ModulateColor != 0x10203040 || copiedSpriteOptions.AlphaRefValue != 123) return 853;\n"
        "  VxSpriteRenderOptions assignedSpriteOptions;\n"
        "  assignedSpriteOptions = copiedSpriteOptions;\n"
        "  if (assignedSpriteOptions.SrcBlendMode != VXBLEND_SRCALPHA || assignedSpriteOptions.DstBlendMode != VXBLEND_INVSRCALPHA) return 854;\n"
        "  VxTransformData transformData;\n"
        "  NativeBuffer@ transformIn = NativeBuffer(12);\n"
        "  NativeBuffer@ transformOut = NativeBuffer(16);\n"
        "  NativeBuffer@ transformScreenBuffer = NativeBuffer(16);\n"
        "  NativeBuffer@ transformClip = NativeBuffer(4);\n"
        "  transformData.InVertices = transformIn.ToPointer();\n"
        "  transformData.OutVertices = transformOut.ToPointer();\n"
        "  transformData.ScreenVertices = transformScreenBuffer.ToPointer();\n"
        "  transformData.ClipFlags = transformClip.ToPointer();\n"
        "  transformData.InStride = 12;\n"
        "  transformData.OutStride = 16;\n"
        "  transformData.ScreenStride = 16;\n"
        "  transformData.m_Offscreen = 1;\n"
        "  if (transformData.InVertices.IsNull() || transformData.OutVertices.IsNull() || transformData.ScreenVertices.IsNull() || transformData.ClipFlags.IsNull()) return 870;\n"
        "  VxTransformData copiedTransformData(transformData);\n"
        "  if (copiedTransformData.InStride != 12 || copiedTransformData.OutStride != 16 || copiedTransformData.m_Offscreen != 1) return 871;\n"
        "  VxTransformData assignedTransformData;\n"
        "  assignedTransformData = copiedTransformData;\n"
        "  if (assignedTransformData.ScreenStride != 16 || assignedTransformData.ClipFlags.IsNull()) return 872;\n"
        "  VxCompressedVector compressed = {0.0f, 1.0f, 0.0f};\n"
        "  VxCompressedVector copiedCompressed;\n"
        "  copiedCompressed = compressed;\n"
        "  if (copiedCompressed.xa != compressed.xa || copiedCompressed.ya != compressed.ya) return 6;\n"
        "  VxVector sourceVector(0.0f, 0.0f, 1.0f);\n"
        "  copiedCompressed = sourceVector;\n"
        "  VxCompressedVector axis;\n"
        "  axis.Set(1.0f, 0.0f, 0.0f);\n"
        "  VxCompressedVector mixed;\n"
        "  mixed.Slerp(0.5f, compressed, axis);\n"
        "  VxCompressedVectorOld oldCompressed = {0.0f, 1.0f, 0.0f};\n"
        "  VxCompressedVectorOld copiedOldCompressed;\n"
        "  copiedOldCompressed = oldCompressed;\n"
        "  if (copiedOldCompressed.xa != oldCompressed.xa || copiedOldCompressed.ya != oldCompressed.ya) return 7;\n"
        "  copiedOldCompressed = sourceVector;\n"
        "  copiedOldCompressed = compressed;\n"
        "  VxCompressedVectorOld oldAxis;\n"
        "  oldAxis.Set(1.0f, 0.0f, 0.0f);\n"
        "  VxCompressedVectorOld oldMixed;\n"
        "  oldMixed.Slerp(0.5f, oldCompressed, oldAxis);\n"
        "  Vx2DVector vec2 = {3.0f, 4.0f};\n"
        "  if (vec2.x != 3.0f || vec2.y != 4.0f) return 8;\n"
        "  if (vec2[0] != 3.0f || vec2[1] != 4.0f) return 9;\n"
        "  vec2[0] = 6.0f;\n"
        "  if (vec2.x != 6.0f) return 10;\n"
        "  if (!(Vx2DVector(1.0f, 9.0f) < Vx2DVector(2.0f, 0.0f))) return 11;\n"
        "  Vx2DVector sum = Vx2DVector(1.0f, 2.0f) + Vx2DVector(3.0f, 4.0f);\n"
        "  if (sum.x != 4.0f || sum.y != 6.0f) return 12;\n"
        "  Vx2DVector norm(3.0f, 4.0f);\n"
        "  norm.Normalize();\n"
        "  if (norm.SquareMagnitude() < 0.99f || norm.SquareMagnitude() > 1.01f) return 13;\n"
        "  if (Vx2DVector(1.0f, 2.0f).Dot(Vx2DVector(3.0f, 4.0f)) != 11.0f) return 14;\n"
        "  Vx2DVector cross = Vx2DVector(2.0f, 3.0f).Cross();\n"
        "  if (cross.x != -3.0f || cross.y != 2.0f) return 15;\n"
        "  VxRect rect = {0.0f, 0.0f, 10.0f, 20.0f};\n"
        "  if (rect.left != 0.0f || rect.top != 0.0f || rect.right != 10.0f || rect.bottom != 20.0f) return 400;\n"
        "  Vx2DVector rectTopLeft(1.0f, 2.0f);\n"
        "  Vx2DVector rectBottomRight(5.0f, 6.0f);\n"
        "  VxRect rectFromVectors(rectTopLeft, rectBottomRight);\n"
        "  if (rectFromVectors.left != 1.0f || rectFromVectors.top != 2.0f || rectFromVectors.right != 5.0f || rectFromVectors.bottom != 6.0f) return 401;\n"
        "  VxRect rectCopy(rect);\n"
        "  if (!(rect == rectCopy)) return 402;\n"
        "  if (!rect.IsInside(Vx2DVector(5.0f, 5.0f))) return 403;\n"
        "  if (rect.IsOutside(VxRect(2.0f, 2.0f, 8.0f, 8.0f))) return 404;\n"
        "  VxRect nullRect;\n"
        "  nullRect.Clear();\n"
        "  if (!nullRect.IsNull()) return 405;\n"
#if CKVERSION != 0x26052005
        "  if (!nullRect.IsEmpty()) return 406;\n"
#endif
        "  VxRect clippedRect(-1.0f, -1.0f, 3.0f, 3.0f);\n"
        "  if (!clippedRect.Clip(VxRect(0.0f, 0.0f, 2.0f, 2.0f))) return 407;\n"
        "  if (clippedRect.left != 0.0f || clippedRect.top != 0.0f || clippedRect.right != 2.0f || clippedRect.bottom != 2.0f) return 408;\n"
        "  Vx2DVector clippedPoint(12.0f, 25.0f);\n"
        "  rect.Clip(clippedPoint);\n"
        "  if (clippedPoint.x != 9.0f || clippedPoint.y != 19.0f) return 409;\n"
        "  rect.Clip(clippedPoint, false);\n"
        "  if (clippedPoint.x != 9.0f || clippedPoint.y != 19.0f) return 410;\n"
        "  VxBbox box = {VxVector(0.0f, 0.0f, 0.0f), VxVector(2.0f, 4.0f, 6.0f)};\n"
        "  if (box.Min.x != 0.0f || box.Max.z != 6.0f) return 16;\n"
        "  VxVector center = box.GetCenter();\n"
        "  if (center.x != 1.0f || center.y != 2.0f || center.z != 3.0f) return 17;\n"
        "  VxVector size = box.GetSize();\n"
        "  if (size.x != 2.0f || size.y != 4.0f || size.z != 6.0f) return 18;\n"
        "  VxBbox sameBox(VxVector(0.0f, 0.0f, 0.0f), VxVector(2.0f, 4.0f, 6.0f));\n"
#if CKVERSION == 0x13022002
        "  if (!(box == sameBox)) return 19;\n"
#endif
        "  if (!box.VectorIn(VxVector(1.0f, 2.0f, 3.0f))) return 20;\n"
        "  box.Merge(VxVector(3.0f, 5.0f, 7.0f));\n"
        "  if (box.Max.x != 3.0f || box.Max.y != 5.0f || box.Max.z != 7.0f) return 21;\n"
        "  box.Intersect(sameBox);\n"
        "  if (box.Max.x != 2.0f || box.Max.y != 4.0f || box.Max.z != 6.0f) return 22;\n"
        "  VxMatrix identity;\n"
        "  identity.SetIdentity();\n"
        "  VxRect transformScreen;\n"
        "  VxRect transformExtents;\n"
        "  VXCLIP_FLAGS transformOrClip;\n"
        "  VXCLIP_FLAGS transformAndClip;\n"
        "  bool transformedBox2D = VxTransformBox2D(identity, sameBox, transformScreen, transformExtents, transformOrClip, transformAndClip);\n"
        "  NativeBuffer@ boxPoints = NativeBuffer(8 * 12);\n"
        "  box.TransformTo(boxPoints.ToPointer(), identity);\n"
        "  VxVector firstPoint;\n"
        "  if (boxPoints.Read(firstPoint) != 12) return 23;\n"
        "  if (firstPoint.x < 0.0f || firstPoint.x > 2.0f || firstPoint.y < 0.0f || firstPoint.y > 4.0f || firstPoint.z < 0.0f || firstPoint.z > 6.0f) return 24;\n"
        "  NativeBuffer@ boxPointsSafe = NativeBuffer(8 * 12);\n"
        "  if (!box.TransformTo(boxPointsSafe, identity)) return 920;\n"
        "  NativeBuffer@ boxFlags = NativeBuffer(2 * 4);\n"
        "  if (!box.ClassifyVertices(2, boxPointsSafe, 12, boxFlags)) return 921;\n"
        "  if (!box.ClassifyVerticesOneAxis(2, boxPointsSafe, 12, 0, boxFlags)) return 922;\n"
        "  VxVector vector = {1.0f, 2.0f, 3.0f};\n"
        "  if (vector.x != 1.0f || vector.y != 2.0f || vector.z != 3.0f) return 25;\n"
        "  if (vector[0] != 1.0f || vector[1] != 2.0f || vector[2] != 3.0f) return 26;\n"
        "  vector[2] = 4.0f;\n"
        "  if (vector.z != 4.0f) return 27;\n"
        "  VxVector scalarAdded = vector + 2.0f;\n"
        "  if (scalarAdded.x != 3.0f || scalarAdded.y != 4.0f || scalarAdded.z != 6.0f) return 28;\n"
        "  VxVector scalarSubtracted = vector - 1.0f;\n"
        "  if (scalarSubtracted.x != 0.0f || scalarSubtracted.y != 1.0f || scalarSubtracted.z != 3.0f) return 29;\n"
        "  if (VxVector(1.0f, 2.0f, 3.0f).Dot(VxVector(4.0f, 5.0f, 6.0f)) != 32.0f) return 30;\n"
        "  VxVector vectorCross = VxVector(1.0f, 0.0f, 0.0f).Cross(VxVector(0.0f, 1.0f, 0.0f));\n"
        "  if (vectorCross.x != 0.0f || vectorCross.y != 0.0f || vectorCross.z != 1.0f) return 31;\n"
        "  VxVector vectorNorm = VxVector(3.0f, 4.0f, 0.0f).Normalize();\n"
        "  if (vectorNorm.SquareMagnitude() < 0.99f || vectorNorm.SquareMagnitude() > 1.01f) return 32;\n"
        "  VxVector reflected = VxVector(0.0f, 1.0f, 0.0f).Reflect(VxVector(0.0f, 1.0f, 0.0f));\n"
        "  if (reflected.x != 0.0f || reflected.y != 1.0f || reflected.z != 0.0f) return 33;\n"
        "  VxVector rotatedByMatrix = vector.Rotate(identity);\n"
        "  if (rotatedByMatrix.x != vector.x || rotatedByMatrix.y != vector.y || rotatedByMatrix.z != vector.z) return 34;\n"
        "  VxVector rotatedByAxis = vector.Rotate(vector.axisY, 0.0f);\n"
        "  if (rotatedByAxis.x != vector.x || rotatedByAxis.y != vector.y || rotatedByAxis.z != vector.z) return 35;\n"
        "  if (vector.axisX.x != 1.0f || vector.axisY.y != 1.0f || vector.axisZ.z != 1.0f) return 36;\n"
        "  VxVector4 vector4 = {1.0f, 2.0f, 3.0f, 4.0f};\n"
        "  if (vector4.x != 1.0f || vector4.y != 2.0f || vector4.z != 3.0f || vector4.w != 4.0f) return 37;\n"
        "  VxVector4 copiedVector4(vector4);\n"
        "  if (copiedVector4.x != 1.0f || copiedVector4.y != 2.0f || copiedVector4.z != 3.0f || copiedVector4.w != 4.0f) return 38;\n"
        "  if (vector4[0] != 1.0f || vector4[1] != 2.0f || vector4[2] != 3.0f || vector4[3] != 4.0f) return 39;\n"
        "  vector4[3] = 8.0f;\n"
        "  if (vector4.w != 8.0f) return 40;\n"
        "  if (!(VxVector4(0.0f, 0.0f, 0.0f, 1.0f) < VxVector4(0.0f, 0.0f, 0.0f, 2.0f))) return 41;\n"
        "  if (!(VxVector4(0.0f, 0.0f, 0.0f, 2.0f) > VxVector4(0.0f, 0.0f, 0.0f, 1.0f))) return 42;\n"
        "  VxVector4 scalarRight = VxVector4(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f;\n"
        "  if (scalarRight.x != 2.0f || scalarRight.y != 4.0f || scalarRight.z != 6.0f || scalarRight.w != 8.0f) return 43;\n"
        "  VxVector4 scalarLeft = 2.0f * VxVector4(1.0f, 2.0f, 3.0f, 4.0f);\n"
        "  if (scalarLeft.x != 2.0f || scalarLeft.y != 4.0f || scalarLeft.z != 6.0f || scalarLeft.w != 8.0f) return 44;\n"
        "  if (VxVector4(1.0f, 2.0f, 3.0f, 4.0f).Dot(VxVector4(4.0f, 5.0f, 6.0f, 7.0f)) != 32.0f) return 45;\n"
        "  if (VxVector4(1.0f, 2.0f, 2.0f, 4.0f).SquareMagnitude() != 9.0f) return 46;\n"
        "  VxVector4 multipliedVector4;\n"
        "  Vx3DMultiplyMatrixVector4(multipliedVector4, identity, VxVector4(1.0f, 2.0f, 3.0f, 4.0f));\n"
        "  if (multipliedVector4.x != 1.0f || multipliedVector4.y != 2.0f || multipliedVector4.z != 3.0f || multipliedVector4.w != 4.0f) return 47;\n"
        "  VxUV defaultUv;\n"
        "  if (defaultUv.u != 0.0f || defaultUv.v != 0.0f) return 48;\n"
        "  VxUV defaultUvCall = VxUV();\n"
        "  if (defaultUvCall.u != 0.0f || defaultUvCall.v != 0.0f) return 49;\n"
        "  VxUV oneArgUv(3.0f);\n"
        "  if (oneArgUv.u != 3.0f || oneArgUv.v != 0.0f) return 50;\n"
        "  VxUV uv(1.0f, 2.0f);\n"
        "  VxUV copiedUv(uv);\n"
        "  if (copiedUv.u != 1.0f || copiedUv.v != 2.0f) return 51;\n"
        "  VxUV uvSum = uv + VxUV(3.0f, 4.0f);\n"
        "  if (uvSum.u != 4.0f || uvSum.v != 6.0f) return 52;\n"
        "  VxUV uvDiff = VxUV(3.0f, 5.0f) - uv;\n"
        "  if (uvDiff.u != 2.0f || uvDiff.v != 3.0f) return 53;\n"
        "  VxUV uvScalarRight = uv * 2.0f;\n"
        "  if (uvScalarRight.u != 2.0f || uvScalarRight.v != 4.0f) return 54;\n"
        "  VxUV uvScalarLeft = 2.0f * uv;\n"
        "  if (uvScalarLeft.u != 2.0f || uvScalarLeft.v != 4.0f) return 55;\n"
        "  VxUV uvDiv = VxUV(4.0f, 8.0f) / 2.0f;\n"
        "  if (uvDiv.u != 2.0f || uvDiv.v != 4.0f) return 56;\n"
        "  uv += VxUV(1.0f, 1.0f);\n"
        "  uv *= 2.0f;\n"
        "  if (uv.u != 4.0f || uv.v != 6.0f) return 57;\n"
        "  VxStridedData stridedDefault;\n"
        "  if (!stridedDefault.Ptr.IsNull() || stridedDefault.Stride != 0) return 58;\n"
        "  NativeBuffer@ sourceBytes = NativeBuffer(36);\n"
        "  NativeBuffer@ destBytes = NativeBuffer(24);\n"
        "  NativeBuffer@ indexBytes = NativeBuffer(8);\n"
        "  if (sourceBytes is null || destBytes is null || indexBytes is null) return 59;\n"
        "  if (sourceBytes.Write(VxVector(1.0f, 0.0f, 0.0f)) != 12) return 60;\n"
        "  if (sourceBytes.Write(VxVector(2.0f, 0.0f, 0.0f)) != 12) return 61;\n"
        "  if (sourceBytes.Write(VxVector(3.0f, 0.0f, 0.0f)) != 12) return 62;\n"
        "  if (!sourceBytes.Seek(0)) return 63;\n"
        "  VxStridedData sourceStride(sourceBytes.ToPointer(), 12);\n"
        "  VxStridedData copiedStride(sourceStride);\n"
        "  if (copiedStride.Ptr.IsNull() || copiedStride.Stride != 12) return 64;\n"
        "  VxStridedData assignedStride;\n"
        "  assignedStride = copiedStride;\n"
        "  if (assignedStride.Ptr.IsNull() || assignedStride.Stride != 12) return 65;\n"
        "  assignedStride.Ptr = destBytes.ToPointer();\n"
        "  assignedStride.Stride = 12;\n"
        "  if (assignedStride.Ptr.IsNull() || assignedStride.Stride != 12) return 66;\n"
        "  if (indexBytes.WriteInt(2) != 4 || indexBytes.WriteInt(0) != 4) return 67;\n"
        "  if (!indexBytes.Seek(0)) return 68;\n"
        "  if (!VxIndexedCopy(assignedStride, sourceStride, 12, indexBytes.ToPointer(), 2)) return 69;\n"
        "  if (!destBytes.Seek(0)) return 70;\n"
        "  VxVector copiedA;\n"
        "  VxVector copiedB;\n"
        "  if (destBytes.Read(copiedA) != 12 || destBytes.Read(copiedB) != 12) return 71;\n"
        "  if (copiedA.x != 3.0f || copiedB.x != 1.0f) return 72;\n"
        "  NativeBuffer@ destBytesSafe = NativeBuffer(24);\n"
        "  if (!VxIndexedCopy(destBytesSafe, 12, sourceBytes, 12, 12, indexBytes, 2)) return 923;\n"
        "  NativeBuffer@ interpA = NativeBuffer(8);\n"
        "  NativeBuffer@ interpB = NativeBuffer(8);\n"
        "  NativeBuffer@ interpOut = NativeBuffer(8);\n"
        "  if (interpA.WriteFloat(1.0f) != 4 || interpA.WriteFloat(3.0f) != 4) return 924;\n"
        "  if (interpB.WriteFloat(5.0f) != 4 || interpB.WriteFloat(7.0f) != 4) return 925;\n"
        "  if (!InterpolateFloatArray(interpOut, interpA, interpB, 0.5f, 2)) return 926;\n"
        "  if (!interpOut.Seek(0)) return 927;\n"
        "  float interpF0;\n"
        "  float interpF1;\n"
        "  if (interpOut.ReadFloat(interpF0) != 4 || interpOut.ReadFloat(interpF1) != 4) return 928;\n"
        "  if (interpF0 != 3.0f || interpF1 != 5.0f) return 929;\n"
        "  NativeBuffer@ interpVecOut = NativeBuffer(24);\n"
        "  if (!InterpolateVectorArray(interpVecOut, sourceBytes, sourceBytes, 0.25f, 2, 12, 12)) return 930;\n"
        "  NativeBuffer@ fillSrc = NativeBuffer(4);\n"
        "  NativeBuffer@ fillDst = NativeBuffer(8);\n"
        "  if (fillSrc.WriteInt(287454020) != 4) return 931;\n"
        "  if (!VxFillStructure(2, fillDst, 4, 4, fillSrc)) return 932;\n"
        "  if (!fillDst.Seek(0)) return 933;\n"
        "  int filled0;\n"
        "  int filled1;\n"
        "  if (fillDst.ReadInt(filled0) != 4 || fillDst.ReadInt(filled1) != 4) return 934;\n"
        "  if (filled0 != 287454020 || filled1 != 287454020) return 935;\n"
        "  NativeBuffer@ copyDstSafe = NativeBuffer(8);\n"
        "  if (!VxCopyStructure(2, copyDstSafe, 4, 4, fillDst, 4)) return 936;\n"
        "  NativeBuffer@ bboxPoints = NativeBuffer(4 * 12);\n"
        "  if (bboxPoints.Write(VxVector(0.0f, 0.0f, 0.0f)) != 12) return 939;\n"
        "  if (bboxPoints.Write(VxVector(1.0f, 0.0f, 0.0f)) != 12) return 940;\n"
        "  if (bboxPoints.Write(VxVector(0.0f, 1.0f, 0.0f)) != 12) return 941;\n"
        "  if (bboxPoints.Write(VxVector(0.0f, 0.0f, 1.0f)) != 12) return 942;\n"
        "  VxMatrix bestFitBox;\n"
        "  if (!VxComputeBestFitBBox(bboxPoints, 12, 4, bestFitBox, 0.0f)) return 943;\n"
        "  VxMatrix matrix;\n"
        "  matrix.SetIdentity();\n"
        "  if (matrix[0].x != 1.0f || matrix[0].y != 0.0f || matrix[1].y != 1.0f || matrix[2].z != 1.0f || matrix[3].w != 1.0f) return 73;\n"
        "  matrix[3].x = 5.0f;\n"
        "  if (matrix[3].x != 5.0f) return 74;\n"
        "  VxMatrix matrixCopy(matrix);\n"
        "  if (!(matrix == matrixCopy)) return 75;\n"
        "  matrixCopy.Clear();\n"
        "  if (matrix == matrixCopy) return 76;\n"
        "  VxMatrix identityCopy;\n"
        "  Vx3DMatrixIdentity(identityCopy);\n"
        "  VxMatrix product = matrix * identityCopy;\n"
        "  if (product[0].x != 1.0f || product[3].x != 5.0f || product[3].w != 1.0f) return 77;\n"
        "  matrix *= identityCopy;\n"
        "  if (matrix[0].x != 1.0f || matrix[3].x != 5.0f || matrix[3].w != 1.0f) return 78;\n"
        "  VxVector matrixVector = identityCopy * VxVector(1.0f, 2.0f, 3.0f);\n"
        "  if (matrixVector.x != 1.0f || matrixVector.y != 2.0f || matrixVector.z != 3.0f) return 79;\n"
        "  VxVector4 matrixVector4 = identityCopy * VxVector4(1.0f, 2.0f, 3.0f, 4.0f);\n"
        "  if (matrixVector4.x != 1.0f || matrixVector4.y != 2.0f || matrixVector4.z != 3.0f || matrixVector4.w != 1.0f) return 80;\n"
        "  VxMatrix multipliedGlobal;\n"
        "  Vx3DMultiplyMatrix(multipliedGlobal, matrix, identityCopy);\n"
        "  if (multipliedGlobal[0].x != 1.0f || multipliedGlobal[3].x != 5.0f || multipliedGlobal[3].w != 1.0f) return 81;\n"
        "  VxRay ray = {VxVector(0.0f, 0.0f, 0.0f), VxVector(0.0f, 1.0f, 0.0f)};\n"
        "  if (ray.origin.x != 0.0f || ray.direction.y != 1.0f) return 300;\n"
        "  VxRay rayFromEndpoints(VxVector(0.0f, 0.0f, 0.0f), VxVector(0.0f, 1.0f, 0.0f));\n"
        "  if (!(ray == rayFromEndpoints)) return 301;\n"
        "  ray.GetOrigin().x = 1.0f;\n"
        "  if (ray.origin.x != 1.0f || ray.GetDirection().y != 1.0f) return 302;\n"
        "  VxRay transformedRay;\n"
        "  rayFromEndpoints.Transform(transformedRay, identityCopy);\n"
        "  if (transformedRay.origin.x != 0.0f || transformedRay.direction.y != 1.0f) return 303;\n"
        "  VxVector interpolatedRayPoint;\n"
        "  rayFromEndpoints.Interpolate(interpolatedRayPoint, 3.0f);\n"
        "  if (interpolatedRayPoint.x != 0.0f || interpolatedRayPoint.y != 3.0f || interpolatedRayPoint.z != 0.0f) return 304;\n"
        "  if (rayFromEndpoints.SquareDistance(VxVector(1.0f, 3.0f, 0.0f)) != 1.0f) return 305;\n"
        "  if (rayFromEndpoints.Distance(VxVector(1.0f, 3.0f, 0.0f)) != 1.0f) return 306;\n"
        "  VxRay boxRay(VxVector(-1.0f, 2.0f, 3.0f), VxVector(3.0f, 2.0f, 3.0f));\n"
        "  if (!VxIntersectRayBox(boxRay, sameBox)) return 307;\n"
        "  VxVector boxInPoint;\n"
        "  VxVector boxOutPoint;\n"
        "  VxVector boxInNormal;\n"
        "  VxVector boxOutNormal;\n"
        "  if (!VxIntersectRayBox(boxRay, sameBox, boxInPoint, boxOutPoint, boxInNormal, boxOutNormal)) return 308;\n"
        "  if (boxInPoint.x < 0.0f || boxInPoint.x > 2.0f) return 309;\n"
        "  if (!VxIntersectSegmentBox(boxRay, sameBox)) return 310;\n"
        "  if (!VxIntersectSegmentBox(boxRay, sameBox, boxInPoint)) return 311;\n"
        "  if (!VxIntersectLineBox(boxRay, sameBox)) return 312;\n"
        "  if (!VxIntersectLineBox(boxRay, sameBox, boxInPoint)) return 313;\n"
        "  VxRay faceRay(VxVector(-1.0f, 1.0f, 1.0f), VxVector(1.0f, 1.0f, 1.0f));\n"
        "  VxVector facePoint;\n"
        "  float faceDist = 0.0f;\n"
        "  if (!VxIntersectRayFace(faceRay, VxVector(0.0f, 0.0f, 0.0f), VxVector(0.0f, 4.0f, 0.0f), VxVector(0.0f, 0.0f, 4.0f), VxVector(-1.0f, 0.0f, 0.0f), facePoint, faceDist)) return 314;\n"
        "  int faceI1 = 0;\n"
        "  int faceI2 = 0;\n"
        "  bool rayFaceCulled = VxIntersectRayFaceCulled(faceRay, VxVector(0.0f, 0.0f, 0.0f), VxVector(0.0f, 4.0f, 0.0f), VxVector(0.0f, 0.0f, 4.0f), VxVector(-1.0f, 0.0f, 0.0f), facePoint, faceDist, faceI1, faceI2);\n"
        "  float rayDistanceT0 = 0.0f;\n"
        "  if (VxPointRaySquareDistance(VxVector(1.0f, 3.0f, 0.0f), rayFromEndpoints, rayDistanceT0) != 1.0f) return 315;\n"
        "  VxSphere sphere(VxVector(1.0f, 2.0f, 3.0f), 2.0f);\n"
        "  if (sphere.Center().x != 1.0f || sphere.Radius() != 2.0f) return 500;\n"
        "  sphere.Center().x = 2.0f;\n"
        "  sphere.Radius() = 3.0f;\n"
        "  if (sphere.Center().x != 2.0f || sphere.Radius() != 3.0f) return 501;\n"
        "  VxSphere sphereCopy(sphere);\n"
#if CKVERSION == 0x13022002
        "  if (!(sphere == sphereCopy)) return 502;\n"
#endif
        "  if (!sphere.IsPointInside(VxVector(2.0f, 2.0f, 3.0f))) return 503;\n"
        "  if (!sphere.IsPointOnSurface(VxVector(5.0f, 2.0f, 3.0f))) return 504;\n"
#if CKVERSION == 0x13022002
        "  if (!VxSphere(VxVector(1.0f, 2.0f, 3.0f), 5.0f).IsBoxTotallyInside(sameBox)) return 505;\n"
#endif
        "  float sphereCollisionT0 = 0.0f;\n"
        "  float sphereCollisionT1 = 0.0f;\n"
        "  bool sphereSphere = VxIntersectSphereSphere(sphere, VxVector(0.0f, 0.0f, 0.0f), sphereCopy, VxVector(0.0f, 0.0f, 0.0f), sphereCollisionT0, sphereCollisionT1);\n"
        "  VxVector raySphereInter1;\n"
        "  VxVector raySphereInter2;\n"
        "  int raySphereCount = VxIntersectRaySphere(faceRay, sphere, raySphereInter1, raySphereInter2);\n"
        "  int sphereAabbResult = VxIntersectSphereAABB(sphere, sameBox);\n"
        "  VxFrustum frustum(VxVector(0.0f, 0.0f, 0.0f), VxVector(1.0f, 0.0f, 0.0f), VxVector(0.0f, 1.0f, 0.0f), VxVector(0.0f, 0.0f, 1.0f), 1.0f, 10.0f, 1.0f, 1.0f);\n"
        "  if (frustum.GetOrigin().x != 0.0f || frustum.GetRight().x != 1.0f || frustum.GetUp().y != 1.0f || frustum.GetDir().z != 1.0f) return 600;\n"
        "  frustum.GetRBound() = 2.0f;\n"
        "  frustum.GetUBound() = 3.0f;\n"
        "  frustum.GetDMin() = 1.0f;\n"
        "  frustum.GetDMax() = 10.0f;\n"
        "  if (ReadConstFrustumRBound(frustum) != 2.0f) return 601;\n"
        "  frustum.Update();\n"
        "  bool frustumInside = frustum.IsInside(VxVector(0.0f, 0.0f, 2.0f));\n"
        "  uint frustumFlags = frustum.Classify(VxVector(0.0f, 0.0f, 2.0f));\n"
        "  float frustumBoxClass = frustum.Classify(sameBox);\n"
        "  float frustumBoxMatrixClass = frustum.Classify(sameBox, identity);\n"
        "  frustum.Transform(identity);\n"
        "  NativeBuffer@ frustumVertices = NativeBuffer(8 * 12);\n"
        "  frustum.ComputeVertices(frustumVertices.ToPointer());\n"
        "  VxVector frustumVertex;\n"
        "  if (frustumVertices.Read(frustumVertex) != 12) return 602;\n"
        "  NativeBuffer@ frustumVerticesSafe = NativeBuffer(8 * 12);\n"
        "  if (!frustum.ComputeVertices(frustumVerticesSafe)) return 937;\n"
        "  bool frustumFace = VxIntersectFrustumFace(frustum, VxVector(0.0f, 0.0f, 2.0f), VxVector(1.0f, 0.0f, 2.0f), VxVector(0.0f, 1.0f, 2.0f));\n"
        "  bool frustumAabb = VxIntersectFrustumAABB(frustum, sameBox);\n"
        "  bool frustumObb = VxIntersectFrustumOBB(frustum, sameBox, identity);\n"
        "  bool frustumBox = VxIntersectFrustumBox(frustum, sameBox, identity);\n"
        "  VxMemoryMappedFile missingMap(\"__ckas_missing_vx_mmf_smoke_7f6e5d99__.bin\");\n"
        "  if (missingMap.IsValid()) return 82;\n"
        "  if (!missingMap.GetBase().IsNull()) return 83;\n"
        "  if (missingMap.GetFileSize() != 0) return 84;\n"
        "  if (missingMap.GetErrorType() != VxMMF_FileOpen) return 85;\n"
        "  NativeBuffer@ keyState = NativeBuffer(256);\n"
        "  if (!keyState.Fill(0, 256)) return 938;\n"
        "  VxScanCodeToAscii(0, keyState);\n"
        "  VxMutex mutex;\n"
        "  mutex.EnterMutex();\n"
        "  mutex.LeaveMutex();\n"
        "  mutex.opPostInc();\n"
        "  mutex.opPostDec();\n"
        "  {\n"
        "    VxMutexLock lock(mutex);\n"
        "  }\n"
        "  VxTimeProfiler timeProfiler;\n"
        "  timeProfiler.Reset();\n"
        "  if (timeProfiler.Current() < 0.0f) return 880;\n"
        "  if (timeProfiler.Split() < 0.0f) return 881;\n"
#if CKVERSION == 0x13022002
        "  VxTimeProfiler assignedTimeProfiler;\n"
        "  assignedTimeProfiler = timeProfiler;\n"
        "  assignedTimeProfiler.Reset();\n"
        "  if (assignedTimeProfiler.Current() < 0.0f) return 882;\n"
#endif
        "  VxSharedLibrary missingLibrary;\n"
        "  if (missingLibrary.Load(\"__ckas_missing_vx_shared_library_smoke_78d3c1f0__.dll\") != 0) return 890;\n"
        "  missingLibrary.Attach(0);\n"
        "  missingLibrary.ReleaseLibrary();\n"
        "  VxSharedLibrary sharedLibrary;\n"
        "  INSTANCE_HANDLE kernel32 = sharedLibrary.Load(\"kernel32.dll\");\n"
        "  if (kernel32 == 0) return 891;\n"
        "  FUNC_PTR getTickCount = sharedLibrary.GetFunctionPtr(\"GetTickCount\");\n"
        "  if (getTickCount == 0) return 892;\n"
        "  sharedLibrary.ReleaseLibrary();\n"
        "  VxOBB obb(box, identity);\n"
        "  if (obb.GetCenter().x != 1.0f || obb.GetCenter().y != 2.0f || obb.GetCenter().z != 3.0f) return 86;\n"
        "  if (obb.GetAxis(0).x != 1.0f || obb.GetAxis(1).y != 1.0f || obb.GetAxis(2).z != 1.0f) return 87;\n"
        "  if (obb.GetExtent(0) != 1.0f || obb.GetExtent(1) != 2.0f || obb.GetExtent(2) != 3.0f) return 88;\n"
        "  if (!obb.VectorIn(VxVector(1.0f, 2.0f, 3.0f))) return 89;\n"
        "  if (!obb.IsBoxInside(sameBox)) return 90;\n"
        "  if (!VxIntersectAABBOBB(sameBox, obb)) return 91;\n"
        "  VxOBB obbCopy(obb);\n"
        "  if (!VxIntersectOBBOBB(obb, obbCopy)) return 92;\n"
        "  obb.GetAxis(0).x = 0.5f;\n"
        "  if (obb.axisX.x != 0.5f) return 93;\n"
        "  obb.GetExtent(0) = 2.0f;\n"
        "  if (obb.extents.x != 2.0f) return 94;\n"
        "  VxPlane plane(VxVector(0.0f, 1.0f, 0.0f), -2.0f);\n"
        "  if (plane.Classify(VxVector(1.0f, 2.0f, 3.0f)) != 0.0f) return 95;\n"
        "  if (plane.Distance(VxVector(1.0f, 5.0f, 3.0f)) != 3.0f) return 96;\n"
        "  VxVector nearest = plane.NearestPoint(VxVector(1.0f, 5.0f, 3.0f));\n"
        "  if (nearest.x != 1.0f || nearest.y != 2.0f || nearest.z != 3.0f) return 97;\n"
        "  if (!VxIntersectBoxPlane(box, plane)) return 98;\n"
        "  if (!VxIntersectBoxPlane(box, identity, plane)) return 99;\n"
        "  array<VxVector> planeAxes = {VxVector(1.0f, 0.0f, 0.0f), VxVector(0.0f, 1.0f, 0.0f), VxVector(0.0f, 0.0f, 1.0f), VxVector(1.0f, 2.0f, 3.0f)};\n"
        "  if (plane.XClassify(planeAxes) != 0.0f) return 100;\n"
        "  VxRay planeRay;\n"
        "  planeRay.origin = VxVector(1.0f, 0.0f, 3.0f);\n"
        "  planeRay.direction = VxVector(0.0f, 1.0f, 0.0f);\n"
        "  VxVector planePoint;\n"
        "  float planeDist = 0.0f;\n"
        "  if (!VxIntersectRayPlane(planeRay, plane, planePoint, planeDist)) return 101;\n"
        "  if (planePoint.y != 2.0f) return 102;\n"
        "  if (!VxIntersectLinePlane(planeRay, plane, planePoint, planeDist)) return 103;\n"
        "  bool segmentPlane = VxIntersectSegmentPlane(planeRay, plane, planePoint, planeDist);\n"
        "  bool segmentPlaneCulled = VxIntersectSegmentPlaneCulled(planeRay, plane, planePoint, planeDist);\n"
        "  bool rayPlaneCulled = VxIntersectRayPlaneCulled(planeRay, plane, planePoint, planeDist);\n"
        "  VxPlane planeX(VxVector(1.0f, 0.0f, 0.0f), 0.0f);\n"
        "  VxPlane planeY(VxVector(0.0f, 1.0f, 0.0f), 0.0f);\n"
        "  VxPlane planeZ(VxVector(0.0f, 0.0f, 1.0f), 0.0f);\n"
        "  VxVector planesPoint;\n"
        "  if (!VxIntersectPlanes(planeX, planeY, planeZ, planesPoint)) return 105;\n"
        "  if (planesPoint.x != 0.0f || planesPoint.y != 0.0f || planesPoint.z != 0.0f) return 106;\n"
        "  VxQuaternion quat = {1.0f, 2.0f, 3.0f, 4.0f};\n"
        "  if (quat.x != 1.0f || quat.y != 2.0f || quat.z != 3.0f || quat.w != 4.0f) return 107;\n"
        "  if (quat[0] != 1.0f || quat[1] != 2.0f || quat[2] != 3.0f || quat[3] != 4.0f) return 108;\n"
        "  quat[3] = 5.0f;\n"
        "  if (quat.w != 5.0f) return 109;\n"
        "  VxQuaternion quatCopy(quat);\n"
        "  if (!(quat == quatCopy)) return 110;\n"
        "  if (quat.Magnitude() != 39.0f) return 111;\n"
        "  if (quat.DotProduct(quat) != 39.0f) return 112;\n"
        "  VxQuaternion identityQuat;\n"
        "  identityQuat.FromMatrix(identity);\n"
        "  VxMatrix quatMatrix;\n"
        "  identityQuat.ToMatrix(quatMatrix);\n"
        "  if (quatMatrix[0].x != 1.0f || quatMatrix[1].y != 1.0f || quatMatrix[2].z != 1.0f) return 113;\n"
        "  VxQuaternion fromMatrixQuat = Vx3DQuaternionFromMatrix(identity);\n"
        "  VxQuaternion multipliedQuat = Vx3DQuaternionMultiply(identityQuat, fromMatrixQuat);\n"
        "  VxQuaternion dividedQuat = Vx3DQuaternionDivide(multipliedQuat, fromMatrixQuat);\n"
        "  VxQuaternion conjugatedQuat = Vx3DQuaternionConjugate(identityQuat);\n"
        "  VxQuaternion snuggleQuat(0.0f, 0.0f, 0.0f, 1.0f);\n"
        "  VxVector snuggleScale(1.0f, 2.0f, 3.0f);\n"
        "  VxQuaternion snuggledQuat = Vx3DQuaternionSnuggle(snuggleQuat, snuggleScale);\n"
        "  if (snuggledQuat.Magnitude() == 0.0f || snuggleScale.x == 0.0f) return 114;\n"
        "  VxQuaternion slerpedQuat = Slerp(0.0f, identityQuat, fromMatrixQuat);\n"
        "  VxQuaternion lndifQuat = LnDif(identityQuat, fromMatrixQuat);\n"
        "  VxQuaternion lnQuat = Ln(identityQuat);\n"
        "  VxQuaternion expQuat = Exp(lnQuat);\n"
        "  return 0;\n"
        "}\n"
        "void OutOfRangeIndex() {\n"
        "  Vx2DVector vec(1.0f, 2.0f);\n"
        "  vec[-1] = 0.0f;\n"
        "}\n"
        "void OutOfRangeVxVectorIndex() {\n"
        "  VxVector vec(1.0f, 2.0f, 3.0f);\n"
        "  vec[3] = 0.0f;\n"
        "}\n"
        "void OutOfRangeVxVector4Index() {\n"
        "  VxVector4 vec(1.0f, 2.0f, 3.0f, 4.0f);\n"
        "  vec[4] = 0.0f;\n"
        "}\n"
        "void OutOfRangeVxMatrixIndex() {\n"
        "  VxMatrix mat;\n"
        "  mat.SetIdentity();\n"
        "  mat[4].x = 0.0f;\n"
        "}\n"
        "void OutOfRangeVxOBBAxis() {\n"
        "  VxOBB obb;\n"
        "  obb.GetAxis(3).x = 0.0f;\n"
        "}\n"
        "void OutOfRangeVxOBBExtent() {\n"
        "  VxOBB obb;\n"
        "  obb.GetExtent(3) = 0.0f;\n"
        "}\n"
        "void OutOfRangeVxQuaternionIndex() {\n"
        "  VxQuaternion quat(1.0f, 2.0f, 3.0f, 4.0f);\n"
        "  quat[4] = 0.0f;\n"
        "}\n"
        "void OutOfRangeXStringIndex() {\n"
        "  XString value(\"x\");\n"
        "  value[1] = 0;\n"
        "}\n";

    asIScriptModule *module = engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
    if (!module) {
        error = "Vx binding self-test could not create a script module.";
        return false;
    }
    int r = module->AddScriptSection("vx-binding-self-test", source);
    if (r < 0) {
        error = "Vx binding self-test could not add script source.";
        engine->DiscardModule(moduleName);
        return false;
    }
    r = module->Build();
    if (r < 0) {
        error = "Vx binding self-test script failed to build.";
        engine->DiscardModule(moduleName);
        return false;
    }
    asIScriptFunction *function = module->GetFunctionByDecl("int Run()");
    if (!function) {
        error = "Vx binding self-test script entry point is unavailable.";
        engine->DiscardModule(moduleName);
        return false;
    }

    asIScriptContext *context = engine->RequestContext();
    if (!context) {
        error = "Vx binding self-test could not create an execution context.";
        engine->DiscardModule(moduleName);
        return false;
    }
    r = context->Prepare(function);
    if (r >= 0) {
        r = context->Execute();
    }
    bool ok = false;
    if (r == asEXECUTION_FINISHED) {
        const int code = static_cast<int>(context->GetReturnDWord());
        if (code == 0) {
            ok = true;
        } else {
            error = "Vx binding self-test script returned " + std::to_string(code) + ".";
        }
    } else if (r == asEXECUTION_EXCEPTION) {
        const char *exception = context->GetExceptionString();
        error = std::string("Vx binding self-test script exception: ") +
                (exception && exception[0] ? exception : "<empty>");
    } else {
        error = "Vx binding self-test script execution failed.";
    }

    auto runExpectedException = [&](const char *decl, const char *expected, const char *label) -> bool {
        context->Unprepare();
        function = module->GetFunctionByDecl(decl);
        if (!function) {
            error = std::string("Vx binding self-test ") + label + " entry point is unavailable.";
            return false;
        }
        r = context->Prepare(function);
        if (r >= 0) {
            r = context->Execute();
        }
        if (r == asEXECUTION_EXCEPTION) {
            const char *exception = context->GetExceptionString();
            const std::string exceptionText = exception ? exception : "";
            if (exceptionText.find(expected) == std::string::npos) {
                error = std::string("Vx binding self-test got unexpected ") + label + " exception: " +
                        (exceptionText.empty() ? std::string("<empty>") : exceptionText);
                return false;
            }
            return true;
        }
        if (r == asEXECUTION_FINISHED) {
            error = std::string("Vx binding self-test ") + label + " did not raise an exception.";
            return false;
        }
        error = std::string("Vx binding self-test ") + label + " execution failed.";
        return false;
    };

    if (ok) {
        ok = runExpectedException("void OutOfRangeIndex()", "Vx2DVector index out of range", "Vx2DVector out-of-range access");
    }
    if (ok) {
        ok = runExpectedException("void OutOfRangeVxVectorIndex()", "VxVector index out of range", "VxVector out-of-range access");
    }
    if (ok) {
        ok = runExpectedException("void OutOfRangeVxVector4Index()", "VxVector4 index out of range", "VxVector4 out-of-range access");
    }
    if (ok) {
        ok = runExpectedException("void OutOfRangeVxMatrixIndex()", "VxMatrix index out of range", "VxMatrix out-of-range access");
    }
    if (ok) {
        ok = runExpectedException("void OutOfRangeVxOBBAxis()", "VxOBB axis index out of range", "VxOBB axis out-of-range access");
    }
    if (ok) {
        ok = runExpectedException("void OutOfRangeVxOBBExtent()", "VxOBB extent index out of range", "VxOBB extent out-of-range access");
    }
    if (ok) {
        ok = runExpectedException("void OutOfRangeVxQuaternionIndex()", "VxQuaternion index out of range", "VxQuaternion out-of-range access");
    }
    if (ok) {
        ok = runExpectedException("void OutOfRangeXStringIndex()", "Out of range", "XString out-of-range access");
    }

    auto expectBuildFailure = [&](const char *script, const char *label) -> bool {
        const char *negativeModuleName = "__CKAS_VxBindingNegativeSelfTest";
        asIScriptModule *negativeModule = engine->GetModule(negativeModuleName, asGM_ALWAYS_CREATE);
        if (!negativeModule) {
            error = std::string("Vx binding self-test could not create negative module for ") + label + ".";
            return false;
        }
        int buildResult = negativeModule->AddScriptSection(label, script);
        if (buildResult >= 0) {
            buildResult = negativeModule->Build();
        }
        engine->DiscardModule(negativeModuleName);
        if (buildResult >= 0) {
            error = std::string("Vx binding self-test unexpectedly built ") + label + ".";
            return false;
        }
        return true;
    };

    if (ok) {
        ok = expectBuildFailure(
            "void RejectDirectoryParserCopy() {\n"
            "  CKDirectoryParser parser(\"__ckas_missing_directory__\", \"*.none\", false);\n"
            "  CKDirectoryParser copied(parser);\n"
            "}\n",
            "CKDirectoryParser copy construction");
    }
    if (ok) {
        ok = expectBuildFailure(
            "void RejectDirectoryParserAssign() {\n"
            "  CKDirectoryParser lhs(\"__ckas_missing_directory__\", \"*.none\", false);\n"
            "  CKDirectoryParser rhs(\"__ckas_missing_directory__\", \"*.none\", false);\n"
            "  lhs = rhs;\n"
            "}\n",
            "CKDirectoryParser assignment");
    }
    if (ok) {
        ok = expectBuildFailure(
            "void RejectDirectoryParserPartialReset() {\n"
            "  CKDirectoryParser parser(\"__ckas_missing_directory__\", \"*.none\", false);\n"
            "  parser.Reset(\"__ckas_missing_directory__\");\n"
            "}\n",
            "CKDirectoryParser partial Reset");
    }

    context->Unprepare();
    engine->ReturnContext(context);
    engine->DiscardModule(moduleName);
    return ok;
}

#if CKVERSION == 0x13022002 || CKVERSION == 0x05082002
static NativePointer GetVxDrawPrimitiveTexCoordPtr(const VxDrawPrimitiveData *self, int stage) {
    if (!self || stage < 0 || stage >= CKRST_MAX_STAGES) {
        return NativePointer();
    }
    return stage == 0 ? NativePointer(self->TexCoordPtr) : NativePointer(self->TexCoordPtrs[stage - 1]);
}

static unsigned int GetVxDrawPrimitiveTexCoordStride(const VxDrawPrimitiveData *self, int stage) {
    if (!self || stage < 0 || stage >= CKRST_MAX_STAGES) {
        return 0;
    }
    return stage == 0 ? self->TexCoordStride : self->TexCoordStrides[stage - 1];
}
#else
static VxStridedData GetVxDrawPrimitiveTexCoord(const VxDrawPrimitiveData *self, int stage) {
    if (!self || stage < 0 || stage >= CKRST_MAX_STAGES) {
        return VxStridedData();
    }
    return stage == 0 ? self->TexCoord : self->TexCoords[stage - 1];
}
#endif
bool RunScriptVxBindingSelfTest(asIScriptEngine *engine, std::string &error) {
    if (!ExecuteVxBindingScriptSmoke(engine, error)) {
        return false;
    }

    asITypeInfo *pathMakerType = engine->GetTypeInfoByDecl("CKPathMaker");
    asITypeInfo *pathSplitterType = engine->GetTypeInfoByDecl("CKPathSplitter");
    if (!pathMakerType || !pathSplitterType) {
        error = "CKPathMaker or CKPathSplitter type is not registered.";
        return false;
    }
    if (!pathMakerType->GetMethodByDecl("string GetFileName() const")) {
        error = "CKPathMaker.GetFileName is not registered.";
        return false;
    }
    if (pathSplitterType->GetMethodByDecl("string GetFileName() const")) {
        error = "CKPathSplitter incorrectly exposes CKPathMaker.GetFileName.";
        return false;
    }
    if (!engine->GetGlobalFunctionByDecl("WIN_HANDLE VxWindowFromPoint(const CKPOINT &in pt)") ||
        !engine->GetGlobalFunctionByDecl("bool VxScreenToClient(WIN_HANDLE win, CKPOINT &inout pt)") ||
        !engine->GetGlobalFunctionByDecl("bool VxClientToScreen(WIN_HANDLE win, CKPOINT &inout pt)")) {
        error = "CKPOINT window function declarations are not registered with safe point passing.";
        return false;
    }
    if (!engine->GetGlobalFunctionByDecl("bool VxDoAlphaBlit(const VxImageDescEx &in dst, NativeBuffer@ alphaValues)") ||
        !engine->GetGlobalFunctionByDecl("bool VxGenerateMipMap(const VxImageDescEx &in src, NativeBuffer@ dst)")) {
        error = "Vx image NativeBuffer overload declarations are not registered.";
        return false;
    }
    if (engine->GetGlobalFunctionByDecl("NativeBuffer@ VxConvertBitmap(BITMAP_HANDLE bitmap, VxImageDescEx &out desc)") != nullptr) {
        error = "VxConvertBitmap self-test found stale NativeBuffer return declaration for unclear ownership.";
        return false;
    }
    for (asUINT i = 0; i < engine->GetGlobalFunctionCount(); ++i) {
        asIScriptFunction *function = engine->GetGlobalFunctionByIndex(i);
        if (!function) {
            continue;
        }
        const std::string decl = function->GetDeclaration();
        if (decl == "WIN_HANDLE VxWindowFromPoint(CKPOINT&in)" ||
            decl == "bool VxScreenToClient(WIN_HANDLE, CKPOINT&out)" ||
            decl == "bool VxClientToScreen(WIN_HANDLE, CKPOINT&out)") {
            error = "CKPOINT window function self-test found stale unsafe declaration: " + decl + ".";
            return false;
        }
    }
    asITypeInfo *directoryParserType = engine->GetTypeInfoByDecl("CKDirectoryParser");
    if (!directoryParserType) {
        error = "CKDirectoryParser type is not registered.";
        return false;
    }
    if (!directoryParserType->GetMethodByDecl("string GetNextFile()")) {
        error = "CKDirectoryParser.GetNextFile is not registered.";
        return false;
    }
    if (!directoryParserType->GetMethodByDecl("void Reset()")) {
        error = "CKDirectoryParser.Reset default overload is not registered.";
        return false;
    }
    if (!directoryParserType->GetMethodByDecl("void Reset(const string &in dir, const string &in fileMask, bool recursive = false)")) {
        error = "CKDirectoryParser.Reset explicit overload is not registered.";
        return false;
    }
    asITypeInfo *imageDescType = engine->GetTypeInfoByDecl("VxImageDescEx");
    if (!imageDescType) {
        error = "VxImageDescEx type is not registered.";
        return false;
    }
    if (!imageDescType->GetMethodByDecl("NativeBuffer@ GetImageBuffer() const") ||
        !imageDescType->GetMethodByDecl("bool SetImageBuffer(NativeBuffer@ buffer)") ||
        !imageDescType->GetMethodByDecl("NativeBuffer@ GetColorMapBuffer() const") ||
        !imageDescType->GetMethodByDecl("bool SetColorMapBuffer(NativeBuffer@ buffer)")) {
        error = "VxImageDescEx NativeBuffer accessors are not registered.";
        return false;
    }

    asITypeInfo *drawPrimitiveType = engine->GetTypeInfoByDecl("VxDrawPrimitiveData");
    if (!drawPrimitiveType) {
        error = "VxDrawPrimitiveData type is not registered.";
        return false;
    }
    for (asUINT i = 0; i < drawPrimitiveType->GetPropertyCount(); ++i) {
        const char *decl = drawPrimitiveType->GetPropertyDeclaration(i, true);
        const std::string propertyDecl = decl ? decl : "";
        if (propertyDecl == "NativePointer PositionPtr" ||
            propertyDecl == "NativePointer NormalPtr" ||
            propertyDecl == "NativePointer ColorPtr" ||
            propertyDecl == "NativePointer SpecularColorPtr" ||
            propertyDecl == "NativePointer TexCoordPtr") {
            error = "VxDrawPrimitiveData self-test found stale writable pointer property: " + propertyDecl + ".";
            return false;
        }
    }

#if CKVERSION == 0x13022002 || CKVERSION == 0x05082002
    if (!drawPrimitiveType->GetMethodByDecl("NativePointer get_PositionPtr() const") ||
        !drawPrimitiveType->GetMethodByDecl("NativePointer get_NormalPtr() const") ||
        !drawPrimitiveType->GetMethodByDecl("NativePointer get_ColorPtr() const") ||
        !drawPrimitiveType->GetMethodByDecl("NativePointer get_SpecularColorPtr() const") ||
        !drawPrimitiveType->GetMethodByDecl("NativePointer get_TexCoordPtr() const") ||
        !drawPrimitiveType->GetMethodByDecl("NativePointer GetTexCoordPtrs(int i) const") ||
        !drawPrimitiveType->GetMethodByDecl("uint GetTexCoordStrides(int i) const")) {
        error = "VxDrawPrimitiveData read-only pointer accessors are not registered.";
        return false;
    }
    if (drawPrimitiveType->GetMethodByDecl("void set_PositionPtr(NativePointer ptr)") ||
        drawPrimitiveType->GetMethodByDecl("void set_NormalPtr(NativePointer ptr)") ||
        drawPrimitiveType->GetMethodByDecl("void set_ColorPtr(NativePointer ptr)") ||
        drawPrimitiveType->GetMethodByDecl("void set_SpecularColorPtr(NativePointer ptr)") ||
        drawPrimitiveType->GetMethodByDecl("void set_TexCoordPtr(NativePointer ptr)")) {
        error = "VxDrawPrimitiveData self-test found stale writable pointer setter.";
        return false;
    }
    VxDrawPrimitiveData data = {};
    char stages[CKRST_MAX_STAGES] = {};
    data.TexCoordPtr = &stages[0];
    data.TexCoordStride = 8;
    for (int stage = 1; stage < CKRST_MAX_STAGES; ++stage) {
        data.TexCoordPtrs[stage - 1] = &stages[stage];
        data.TexCoordStrides[stage - 1] = static_cast<unsigned int>(8 + stage);
    }

    if (GetVxDrawPrimitiveTexCoordPtr(&data, 0).Get() != &stages[0] ||
        GetVxDrawPrimitiveTexCoordStride(&data, 0) != 8) {
        error = "VxDrawPrimitiveData stage 0 texture coordinate access is invalid.";
        return false;
    }
    if (GetVxDrawPrimitiveTexCoordPtr(&data, 1).Get() != &stages[1] ||
        GetVxDrawPrimitiveTexCoordStride(&data, 1) != 9) {
        error = "VxDrawPrimitiveData stage 1 texture coordinate access is invalid.";
        return false;
    }
    if (GetVxDrawPrimitiveTexCoordPtr(&data, CKRST_MAX_STAGES - 1).Get() != &stages[CKRST_MAX_STAGES - 1] ||
        GetVxDrawPrimitiveTexCoordStride(&data, CKRST_MAX_STAGES - 1) != static_cast<unsigned int>(8 + CKRST_MAX_STAGES - 1)) {
        error = "VxDrawPrimitiveData last texture coordinate stage is unreachable.";
        return false;
    }
    if (GetVxDrawPrimitiveTexCoordPtr(&data, -1).Get() ||
        GetVxDrawPrimitiveTexCoordPtr(&data, CKRST_MAX_STAGES).Get() ||
        GetVxDrawPrimitiveTexCoordStride(&data, -1) != 0 ||
        GetVxDrawPrimitiveTexCoordStride(&data, CKRST_MAX_STAGES) != 0) {
        error = "VxDrawPrimitiveData out-of-range texture coordinate stages are not rejected.";
        return false;
    }

    Vx3DCapsDesc caps = {};
    caps.CKRasterizerSpecificCaps = 0x12345678u;
    Vx3DCapsDesc copiedCaps(caps);
    Vx3DCapsDesc assignedCaps = {};
    assignedCaps = copiedCaps;
    if (copiedCaps.CKRasterizerSpecificCaps != 0x12345678u ||
        assignedCaps.CKRasterizerSpecificCaps != 0x12345678u) {
        error = "Vx3DCapsDesc CKRasterizerSpecificCaps copy/assignment is invalid.";
        return false;
    }
#else
    VxDrawPrimitiveData data = {};
    char stages[CKRST_MAX_STAGES] = {};
    data.TexCoord.Ptr = &stages[0];
    data.TexCoord.Stride = 8;
    for (int stage = 1; stage < CKRST_MAX_STAGES; ++stage) {
        data.TexCoords[stage - 1].Ptr = &stages[stage];
        data.TexCoords[stage - 1].Stride = static_cast<unsigned int>(8 + stage);
    }

    if (GetVxDrawPrimitiveTexCoord(&data, 0).Ptr != &stages[0] ||
        GetVxDrawPrimitiveTexCoord(&data, 0).Stride != 8 ||
        GetVxDrawPrimitiveTexCoord(&data, 1).Ptr != &stages[1] ||
        GetVxDrawPrimitiveTexCoord(&data, 1).Stride != 9 ||
        GetVxDrawPrimitiveTexCoord(&data, CKRST_MAX_STAGES - 1).Ptr != &stages[CKRST_MAX_STAGES - 1] ||
        GetVxDrawPrimitiveTexCoord(&data, CKRST_MAX_STAGES - 1).Stride != static_cast<unsigned int>(8 + CKRST_MAX_STAGES - 1) ||
        GetVxDrawPrimitiveTexCoord(&data, -1).Ptr ||
        GetVxDrawPrimitiveTexCoord(&data, CKRST_MAX_STAGES).Ptr) {
        error = "VxDrawPrimitiveData texture coordinate stage access is invalid.";
        return false;
    }

    Vx3DCapsDesc caps = {};
    caps.CKRasterizerSpecificCaps = 0x12345678u;
    Vx3DCapsDesc copiedCaps(caps);
    Vx3DCapsDesc assignedCaps = {};
    assignedCaps = copiedCaps;
    if (copiedCaps.CKRasterizerSpecificCaps != 0x12345678u ||
        assignedCaps.CKRasterizerSpecificCaps != 0x12345678u) {
        error = "Vx3DCapsDesc CKRasterizerSpecificCaps copy/assignment is invalid.";
        return false;
    }
#endif
    return true;
}
