import 'dart:ffi';
import 'dart:io';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

/// Mirrors the C struct EyetrackResultC from eyetrack_ffi.h.
final class EyetrackResultC extends Struct {
  @Int32()
  external int detected;

  @Float()
  external double gazeX;

  @Float()
  external double gazeY;

  @Float()
  external double confidence;

  @Int32()
  external int blinkType;

  @Float()
  external double earLeft;

  @Float()
  external double earRight;

  @Float()
  external double irisRatioH;

  @Float()
  external double irisRatioV;

  @Float()
  external double leftPupilX;

  @Float()
  external double leftPupilY;

  @Float()
  external double rightPupilX;

  @Float()
  external double rightPupilY;

  @Float()
  external double faceX;

  @Float()
  external double faceY;

  @Float()
  external double faceW;

  @Float()
  external double faceH;
}

// Native function typedefs
typedef _EyetrackCreateNative = Pointer<Void> Function(Pointer<Utf8>);
typedef _EyetrackCreateDart = Pointer<Void> Function(Pointer<Utf8>);

typedef _EyetrackDestroyNative = Void Function(Pointer<Void>);
typedef _EyetrackDestroyDart = void Function(Pointer<Void>);

typedef _EyetrackProcessFrameNative = EyetrackResultC Function(
    Pointer<Void>, Pointer<Uint8>, Int32, Int32, Int32, Int32);
typedef _EyetrackProcessFrameDart = EyetrackResultC Function(
    Pointer<Void>, Pointer<Uint8>, int, int, int, int);

typedef _EyetrackSetSmoothingNative = Void Function(Pointer<Void>, Float);
typedef _EyetrackSetSmoothingDart = void Function(Pointer<Void>, double);

typedef _EyetrackResetNative = Void Function(Pointer<Void>);
typedef _EyetrackResetDart = void Function(Pointer<Void>);

/// Low-level FFI wrapper around the C++ eyetrack library.
class EyetrackFFI {
  DynamicLibrary? _lib;
  Pointer<Void>? _handle;
  bool _initialized = false;

  late _EyetrackCreateDart _create;
  late _EyetrackDestroyDart _destroy;
  late _EyetrackProcessFrameDart _processFrame;
  late _EyetrackSetSmoothingDart _setSmoothing;
  late _EyetrackResetDart _reset;

  bool get isInitialized => _initialized;

  /// Initialize the eye tracker. Loads the dylib and the dlib model.
  /// [modelDir] should point to the directory containing
  /// shape_predictor_68_face_landmarks.dat.
  void initialize(String modelDir) {
    if (_initialized) return;

    final dylibPath = _findDylibPath();
    if (dylibPath == null) {
      throw StateError('libeyetrack_ffi.dylib not found in app bundle');
    }

    _lib = DynamicLibrary.open(dylibPath);

    _create = _lib!.lookupFunction<_EyetrackCreateNative, _EyetrackCreateDart>(
        'eyetrack_create');
    _destroy =
        _lib!.lookupFunction<_EyetrackDestroyNative, _EyetrackDestroyDart>(
            'eyetrack_destroy');
    _processFrame = _lib!.lookupFunction<_EyetrackProcessFrameNative,
        _EyetrackProcessFrameDart>('eyetrack_process_frame');
    _setSmoothing = _lib!.lookupFunction<_EyetrackSetSmoothingNative,
        _EyetrackSetSmoothingDart>('eyetrack_set_smoothing');
    _reset = _lib!.lookupFunction<_EyetrackResetNative, _EyetrackResetDart>(
        'eyetrack_reset');

    final modelDirPtr = modelDir.toNativeUtf8();
    _handle = _create(modelDirPtr);
    calloc.free(modelDirPtr);

    if (_handle == nullptr || _handle == Pointer<Void>.fromAddress(0)) {
      throw StateError('Failed to create eyetrack instance. Check model path: $modelDir');
    }

    _initialized = true;
  }

  /// Process a single frame and return the result.
  /// [pixelData]: raw ARGB8888 pixel buffer
  /// [pixelFormat]: 0=BGRA, 1=ARGB
  EyetrackResultC processFrame(
      Uint8List pixelData, int width, int height, int bytesPerRow,
      {int pixelFormat = 1}) {
    if (!_initialized || _handle == null) {
      throw StateError('EyetrackFFI not initialized');
    }

    // Allocate native memory and copy pixel data
    final ptr = calloc<Uint8>(pixelData.length);
    ptr.asTypedList(pixelData.length).setAll(0, pixelData);

    final result =
        _processFrame(_handle!, ptr, width, height, bytesPerRow, pixelFormat);

    calloc.free(ptr);
    return result;
  }

  /// Set EMA smoothing alpha.
  void setSmoothing(double alpha) {
    if (!_initialized || _handle == null) return;
    _setSmoothing(_handle!, alpha);
  }

  /// Reset smoother and blink state.
  void reset() {
    if (!_initialized || _handle == null) return;
    _reset(_handle!);
  }

  /// Dispose and free the native handle.
  void dispose() {
    if (_initialized && _handle != null) {
      _destroy(_handle!);
      _handle = null;
      _initialized = false;
    }
  }

  /// Find the dylib path relative to the executable.
  static String? _findDylibPath() {
    // In a macOS .app bundle: Contents/MacOS/client (executable)
    // dylib is at: Contents/Frameworks/libeyetrack_ffi.dylib
    final execPath = Platform.resolvedExecutable;
    final macosDir = File(execPath).parent.path;
    final contentsDir = Directory(macosDir).parent.path;

    // Try Frameworks directory first
    final frameworksPath =
        '$contentsDir/Frameworks/libeyetrack_ffi.dylib';
    if (File(frameworksPath).existsSync()) return frameworksPath;

    // Try alongside executable
    final execDirPath = '$macosDir/libeyetrack_ffi.dylib';
    if (File(execDirPath).existsSync()) return execDirPath;

    // Try current directory (for development)
    if (File('libeyetrack_ffi.dylib').existsSync()) {
      return 'libeyetrack_ffi.dylib';
    }

    return null;
  }

  /// Resolve the model directory path inside the app bundle.
  static String? findModelDir() {
    final execPath = Platform.resolvedExecutable;
    final macosDir = File(execPath).parent.path;
    final contentsDir = Directory(macosDir).parent.path;

    final modelDir = '$contentsDir/Resources/models';
    if (Directory(modelDir).existsSync()) return modelDir;

    // Fallback: check relative to project root (development)
    final devModelDir = '${Directory.current.path}/models';
    if (Directory(devModelDir).existsSync()) return devModelDir;

    return null;
  }
}
