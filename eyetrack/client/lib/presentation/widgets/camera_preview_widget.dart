import 'package:camera/camera.dart';
import 'package:flutter/material.dart';

import 'package:eyetrack/core/camera_service.dart';

class CameraPreviewWidget extends StatelessWidget {
  const CameraPreviewWidget({
    super.key,
    required this.cameraService,
    this.placeholder,
  });

  final CameraService cameraService;
  final Widget? placeholder;

  @override
  Widget build(BuildContext context) {
    return StreamBuilder<CameraStatus>(
      stream: cameraService.statusStream,
      initialData: cameraService.status,
      builder: (context, snapshot) {
        final status = snapshot.data ?? CameraStatus.uninitialized;

        switch (status) {
          case CameraStatus.uninitialized:
          case CameraStatus.error:
            return placeholder ?? _buildPlaceholder(context, status);
          case CameraStatus.initializing:
            return const Center(child: CircularProgressIndicator());
          case CameraStatus.ready:
          case CameraStatus.streaming:
            final controller = cameraService.controller;
            if (controller == null || !controller.value.isInitialized) {
              return placeholder ?? _buildPlaceholder(context, status);
            }
            return ClipRRect(
              borderRadius: BorderRadius.circular(12),
              child: AspectRatio(
                aspectRatio: controller.value.aspectRatio,
                child: CameraPreview(controller),
              ),
            );
        }
      },
    );
  }

  Widget _buildPlaceholder(BuildContext context, CameraStatus status) {
    final icon = status == CameraStatus.error
        ? Icons.error_outline
        : Icons.camera_alt_outlined;
    final color = status == CameraStatus.error
        ? Theme.of(context).colorScheme.error
        : Theme.of(context).colorScheme.onSurfaceVariant;

    return Container(
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surfaceContainerHighest,
        borderRadius: BorderRadius.circular(12),
      ),
      child: Center(
        child: Icon(icon, size: 64, color: color),
      ),
    );
  }
}
