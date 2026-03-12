import Cocoa
import FlutterMacOS
import Vision
import Accelerate

/// Native macOS eye tracker using Apple Vision framework + pixel-level iris detection.
/// Based on the GazeTracking approach: crop eye region → threshold → find dark iris blob → compute ratio.
class EyeTrackerPlugin: NSObject, FlutterPlugin {

    // Auto-calibration: accumulate best thresholds over first N frames
    private var calibrationFrames = 0
    private let calibrationTarget = 20
    private var leftThresholdSum: Double = 0
    private var rightThresholdSum: Double = 0
    private var calibratedThreshold: UInt8 = 70  // default until calibration completes
    private var isCalibrated = false

    static func register(with registrar: FlutterPluginRegistrar) {
        let channel = FlutterMethodChannel(
            name: "eyetrack/native_eye_tracker",
            binaryMessenger: registrar.messenger
        )
        let instance = EyeTrackerPlugin()
        registrar.addMethodCallDelegate(instance, channel: channel)
    }

    func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        switch call.method {
        case "detectGazeRaw":
            guard let args = call.arguments as? [String: Any],
                  let imageData = (args["imageData"] as? FlutterStandardTypedData)?.data,
                  let width = args["width"] as? Int,
                  let height = args["height"] as? Int,
                  let bytesPerRow = args["bytesPerRow"] as? Int else {
                result(FlutterError(code: "INVALID_ARGS", message: "Missing args", details: nil))
                return
            }
            detectGazeFromRaw(data: imageData, width: width, height: height, bytesPerRow: bytesPerRow, result: result)
        case "detectGaze":
            guard let args = call.arguments as? [String: Any],
                  let imageData = (args["imageData"] as? FlutterStandardTypedData)?.data else {
                result(FlutterError(code: "INVALID_ARGS", message: "Missing imageData", details: nil))
                return
            }
            detectGazeFromEncoded(imageData: imageData, result: result)
        default:
            result(FlutterMethodNotImplemented)
        }
    }

    // MARK: - Raw ARGB8888 frame processing

    private func detectGazeFromRaw(data: Data, width: Int, height: Int, bytesPerRow: Int, result: @escaping FlutterResult) {
        DispatchQueue.global(qos: .userInteractive).async { [weak self] in
            guard let self = self else { return }

            let colorSpace = CGColorSpaceCreateDeviceRGB()
            guard let provider = CGDataProvider(data: data as CFData),
                  let cgImage = CGImage(
                    width: width,
                    height: height,
                    bitsPerComponent: 8,
                    bitsPerPixel: 32,
                    bytesPerRow: bytesPerRow,
                    space: colorSpace,
                    bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.first.rawValue),
                    provider: provider,
                    decode: nil,
                    shouldInterpolate: false,
                    intent: .defaultIntent
                  ) else {
                DispatchQueue.main.async {
                    result(["detected": false] as [String: Any])
                }
                return
            }

            self.runVision(cgImage: cgImage, result: result)
        }
    }

    // MARK: - Encoded image processing

    private func detectGazeFromEncoded(imageData: Data, result: @escaping FlutterResult) {
        guard let image = NSImage(data: imageData),
              let cgImage = image.cgImage(forProposedRect: nil, context: nil, hints: nil) else {
            result(["detected": false] as [String: Any])
            return
        }

        DispatchQueue.global(qos: .userInteractive).async { [weak self] in
            self?.runVision(cgImage: cgImage, result: result)
        }
    }

    // MARK: - Vision processing

    private func runVision(cgImage: CGImage, result: @escaping FlutterResult) {
        let request = VNDetectFaceLandmarksRequest()

        let handler = VNImageRequestHandler(cgImage: cgImage, options: [:])
        do {
            try handler.perform([request])
        } catch {
            DispatchQueue.main.async {
                result(["detected": false] as [String: Any])
            }
            return
        }

        guard let observations = request.results,
              let face = observations.first else {
            DispatchQueue.main.async {
                result(["detected": false] as [String: Any])
            }
            return
        }

        let gazeData = self.computeGaze(from: face, cgImage: cgImage)
        DispatchQueue.main.async {
            result(gazeData)
        }
    }

    // MARK: - Gaze computation (GazeTracking approach)
    //
    // Algorithm (ported from antoinelame/GazeTracking):
    //   1. Use Vision landmarks to locate eye regions
    //   2. Crop eye region from the actual image pixels
    //   3. Convert to grayscale, apply threshold to isolate dark iris
    //   4. Find centroid of the dark blob (iris)
    //   5. Compute iris ratio = iris_x / eye_width (0.0-1.0)
    //   6. Map iris ratios to screen gaze coordinates

    private func computeGaze(from face: VNFaceObservation, cgImage: CGImage) -> [String: Any] {
        let faceBBox = face.boundingBox
        let imgW = CGFloat(cgImage.width)
        let imgH = CGFloat(cgImage.height)

        guard let landmarks = face.landmarks,
              let leftEyePts = landmarks.leftEye,
              let rightEyePts = landmarks.rightEye else {
            let headX = faceBBox.origin.x + faceBBox.width / 2.0
            let headY = faceBBox.origin.y + faceBBox.height / 2.0
            return [
                "detected": true,
                "gazeX": 1.0 - headX,
                "gazeY": 1.0 - headY,
                "confidence": Double(face.confidence) * 0.3,
                "vecX": 0.0, "vecY": 0.0,
                "earLeft": 0.3, "earRight": 0.3,
                "leftEyeCenter": [0.5, 0.5],
                "rightEyeCenter": [0.5, 0.5],
                "leftPupilPos": [0.5, 0.5],
                "rightPupilPos": [0.5, 0.5],
                "faceBox": [faceBBox.origin.x, faceBBox.origin.y,
                            faceBBox.width, faceBBox.height],
                "yaw": 0.0,
                "pupilOffX": 0.0, "pupilOffY": 0.0,
                "irisRatioH": 0.5, "irisRatioV": 0.5,
            ] as [String: Any]
        }

        let leCenter = centroid(of: leftEyePts)
        let reCenter = centroid(of: rightEyePts)
        let leftEyeImg = toImageCoords(leCenter, in: faceBBox)
        let rightEyeImg = toImageCoords(reCenter, in: faceBBox)

        let earLeft = computeEAR(points: leftEyePts)
        let earRight = computeEAR(points: rightEyePts)

        let yawRad = face.yaw?.doubleValue ?? 0.0

        // --- Pixel-level iris detection ---
        // Crop each eye region from the image and find the iris centroid via thresholding

        let leftIris = detectIrisInEye(
            eyePts: leftEyePts, faceBBox: faceBBox,
            cgImage: cgImage, imgW: imgW, imgH: imgH
        )
        let rightIris = detectIrisInEye(
            eyePts: rightEyePts, faceBBox: faceBBox,
            cgImage: cgImage, imgW: imgW, imgH: imgH
        )

        // Iris ratio: where the iris sits within the eye (0.0=left edge, 1.0=right edge)
        // Average both eyes for stability
        var irisRatioH: Double = 0.5
        var irisRatioV: Double = 0.5
        var hasIris = false

        if let li = leftIris, let ri = rightIris {
            irisRatioH = (li.ratioH + ri.ratioH) / 2.0
            irisRatioV = (li.ratioV + ri.ratioV) / 2.0
            hasIris = true
        } else if let li = leftIris {
            irisRatioH = li.ratioH
            irisRatioV = li.ratioV
            hasIris = true
        } else if let ri = rightIris {
            irisRatioH = ri.ratioH
            irisRatioV = ri.ratioV
            hasIris = true
        }

        // --- Map iris ratios to screen coordinates ---
        // In the raw camera image (not mirrored):
        //   - Subject looks LEFT → iris moves to THEIR left → in camera, iris appears on RIGHT
        //     → irisRatioH increases → gazeX should decrease (left on screen)
        //   - Subject looks UP → iris moves up → in image, iris is higher
        //     → irisRatioV decreases (iris near top of eye) → gazeY should decrease (top of screen)
        //
        // So: gazeX = 1.0 - irisRatioH (invert horizontal)
        //     gazeY = irisRatioV (direct mapping, already top=0)

        var gazeX: Double
        var gazeY: Double
        var confidence: Double

        if hasIris {
            // Primary: iris ratio (full range 0-1 maps to screen 0-1)
            gazeX = 1.0 - irisRatioH
            gazeY = irisRatioV

            // Secondary: add yaw for head rotation compensation
            // yaw < 0 when subject turns to their left → gazeX should decrease
            gazeX += yawRad * 0.3

            confidence = Double(face.confidence) * 0.8 + 0.2
        } else {
            // Fallback: head position only
            let headX = faceBBox.origin.x + faceBBox.width / 2.0
            let headY = faceBBox.origin.y + faceBBox.height / 2.0
            gazeX = 1.0 - headX
            gazeY = 1.0 - headY
            confidence = Double(face.confidence) * 0.3
        }

        let vecX = hasIris ? (0.5 - irisRatioH) * 2.0 : 0.0
        let vecY = hasIris ? (irisRatioV - 0.5) * 2.0 : 0.0

        gazeX = min(max(gazeX, 0.0), 1.0)
        gazeY = min(max(gazeY, 0.0), 1.0)

        // Pupil positions in image coords (for overlay)
        let leftPupilImg: [Double]
        let rightPupilImg: [Double]
        if let li = leftIris {
            leftPupilImg = li.imgCoords
        } else {
            leftPupilImg = leftEyeImg
        }
        if let ri = rightIris {
            rightPupilImg = ri.imgCoords
        } else {
            rightPupilImg = rightEyeImg
        }

        return [
            "detected": true,
            "gazeX": gazeX,
            "gazeY": gazeY,
            "confidence": confidence,
            "vecX": vecX,
            "vecY": vecY,
            "earLeft": earLeft,
            "earRight": earRight,
            "leftEyeCenter": leftEyeImg,
            "rightEyeCenter": rightEyeImg,
            "leftPupilPos": leftPupilImg,
            "rightPupilPos": rightPupilImg,
            "faceBox": [faceBBox.origin.x, faceBBox.origin.y,
                        faceBBox.width, faceBBox.height],
            "yaw": yawRad,
            "pupilOffX": irisRatioH,
            "pupilOffY": irisRatioV,
            "irisRatioH": irisRatioH,
            "irisRatioV": irisRatioV,
        ] as [String: Any]
    }

    // MARK: - Iris detection via pixel thresholding

    struct IrisResult {
        let ratioH: Double   // horizontal: 0.0=left edge of eye, 1.0=right edge
        let ratioV: Double   // vertical: 0.0=top of eye, 1.0=bottom
        let imgCoords: [Double]  // iris center in normalized image coords
    }

    private func detectIrisInEye(
        eyePts: VNFaceLandmarkRegion2D,
        faceBBox: CGRect,
        cgImage: CGImage,
        imgW: CGFloat, imgH: CGFloat
    ) -> IrisResult? {
        let points = eyePts.normalizedPoints
        guard points.count >= 4 else { return nil }

        // Convert eye landmarks from face-relative to pixel coords
        var pixelPoints: [CGPoint] = []
        for p in points {
            let imgX = (faceBBox.origin.x + p.x * faceBBox.width) * imgW
            // Vision Y is bottom-up, CGImage Y is top-down
            let imgY = (1.0 - (faceBBox.origin.y + p.y * faceBBox.height)) * imgH
            pixelPoints.append(CGPoint(x: imgX, y: imgY))
        }

        // Bounding box of eye in pixels with margin
        let xs = pixelPoints.map { $0.x }
        let ys = pixelPoints.map { $0.y }
        let margin: CGFloat = 5
        let minX = max(0, (xs.min() ?? 0) - margin)
        let minY = max(0, (ys.min() ?? 0) - margin)
        let maxX = min(imgW, (xs.max() ?? imgW) + margin)
        let maxY = min(imgH, (ys.max() ?? imgH) + margin)

        let cropW = Int(maxX - minX)
        let cropH = Int(maxY - minY)
        guard cropW > 4, cropH > 4 else { return nil }

        // Crop the eye region from the image
        let cropRect = CGRect(x: minX, y: minY, width: CGFloat(cropW), height: CGFloat(cropH))
        guard let croppedImage = cgImage.cropping(to: cropRect) else { return nil }

        // Convert to grayscale pixel buffer
        guard let grayPixels = toGrayscale(croppedImage) else { return nil }

        // Create eye mask polygon (only process pixels inside the eye outline)
        let eyeMask = createEyeMask(
            points: pixelPoints, cropOrigin: CGPoint(x: minX, y: minY),
            width: cropW, height: cropH
        )

        // Auto-calibrate or use calibrated threshold
        let threshold: UInt8
        if !isCalibrated {
            threshold = findBestThreshold(grayPixels: grayPixels, mask: eyeMask, width: cropW, height: cropH)
            updateCalibration(threshold: threshold)
        } else {
            threshold = calibratedThreshold
        }

        // Apply threshold and find iris centroid
        guard let irisCentroid = findIrisCentroid(
            grayPixels: grayPixels, mask: eyeMask,
            threshold: threshold, width: cropW, height: cropH
        ) else { return nil }

        // Compute ratios within the eye bounding box
        let eyeWidth = maxX - minX
        let eyeHeight = maxY - minY
        guard eyeWidth > 0, eyeHeight > 0 else { return nil }

        let ratioH = Double(irisCentroid.x) / Double(eyeWidth)
        let ratioV = Double(irisCentroid.y) / Double(eyeHeight)

        // Convert back to normalized image coords
        let imgCoordX = Double(minX + irisCentroid.x) / Double(imgW)
        let imgCoordY = Double(minY + irisCentroid.y) / Double(imgH)

        return IrisResult(
            ratioH: min(max(ratioH, 0.0), 1.0),
            ratioV: min(max(ratioV, 0.0), 1.0),
            imgCoords: [imgCoordX, 1.0 - imgCoordY]  // convert back to Vision coords (bottom-up)
        )
    }

    // MARK: - Image processing helpers

    /// Convert CGImage to grayscale UInt8 buffer
    private func toGrayscale(_ image: CGImage) -> [UInt8]? {
        let w = image.width
        let h = image.height
        var grayPixels = [UInt8](repeating: 0, count: w * h)

        guard let context = CGContext(
            data: &grayPixels,
            width: w, height: h,
            bitsPerComponent: 8,
            bytesPerRow: w,
            space: CGColorSpaceCreateDeviceGray(),
            bitmapInfo: CGImageAlphaInfo.none.rawValue
        ) else { return nil }

        context.draw(image, in: CGRect(x: 0, y: 0, width: w, height: h))
        return grayPixels
    }

    /// Create a binary mask from the eye polygon
    private func createEyeMask(points: [CGPoint], cropOrigin: CGPoint, width: Int, height: Int) -> [Bool] {
        var mask = [Bool](repeating: false, count: width * height)

        // Translate points to crop coordinates
        let localPoints = points.map { CGPoint(x: $0.x - cropOrigin.x, y: $0.y - cropOrigin.y) }

        // Fill polygon using scanline
        for y in 0..<height {
            let fy = CGFloat(y) + 0.5
            var intersections: [CGFloat] = []

            for i in 0..<localPoints.count {
                let j = (i + 1) % localPoints.count
                let p1 = localPoints[i]
                let p2 = localPoints[j]

                if (p1.y <= fy && p2.y > fy) || (p2.y <= fy && p1.y > fy) {
                    let t = (fy - p1.y) / (p2.y - p1.y)
                    intersections.append(p1.x + t * (p2.x - p1.x))
                }
            }

            intersections.sort()
            for k in stride(from: 0, to: intersections.count - 1, by: 2) {
                let xStart = max(0, Int(intersections[k]))
                let xEnd = min(width - 1, Int(intersections[k + 1]))
                for x in xStart...xEnd {
                    mask[y * width + x] = true
                }
            }
        }

        return mask
    }

    /// Find best binarization threshold (GazeTracking calibration approach)
    /// Searches for threshold where iris area ≈ 48% of the masked eye region
    private func findBestThreshold(grayPixels: [UInt8], mask: [Bool], width: Int, height: Int) -> UInt8 {
        let totalMasked = mask.filter { $0 }.count
        guard totalMasked > 0 else { return 70 }

        let targetRatio = 0.48
        var bestThreshold: UInt8 = 70
        var bestDiff = Double.greatestFiniteMagnitude

        for t in stride(from: UInt8(5), through: 100, by: 5) {
            var darkCount = 0
            for i in 0..<(width * height) {
                if mask[i] && grayPixels[i] < t {
                    darkCount += 1
                }
            }
            let ratio = Double(darkCount) / Double(totalMasked)
            let diff = abs(ratio - targetRatio)
            if diff < bestDiff {
                bestDiff = diff
                bestThreshold = t
            }
        }

        return bestThreshold
    }

    /// Update auto-calibration with a new threshold sample
    private func updateCalibration(threshold: UInt8) {
        leftThresholdSum += Double(threshold)
        calibrationFrames += 1

        if calibrationFrames >= calibrationTarget {
            calibratedThreshold = UInt8(leftThresholdSum / Double(calibrationFrames))
            isCalibrated = true
        }
    }

    /// Find the centroid of the dark (iris) region in the thresholded eye image
    private func findIrisCentroid(
        grayPixels: [UInt8], mask: [Bool],
        threshold: UInt8, width: Int, height: Int
    ) -> CGPoint? {
        var sumX: Double = 0
        var sumY: Double = 0
        var count: Double = 0

        for y in 0..<height {
            for x in 0..<width {
                let idx = y * width + x
                if mask[idx] && grayPixels[idx] < threshold {
                    sumX += Double(x)
                    sumY += Double(y)
                    count += 1
                }
            }
        }

        guard count > 0 else { return nil }
        return CGPoint(x: sumX / count, y: sumY / count)
    }

    // MARK: - Vision landmark helpers

    private func toImageCoords(_ point: CGPoint, in bbox: CGRect) -> [Double] {
        return [
            Double(bbox.origin.x + point.x * bbox.width),
            Double(bbox.origin.y + point.y * bbox.height)
        ]
    }

    private func centroid(of region: VNFaceLandmarkRegion2D) -> CGPoint {
        let points = region.normalizedPoints
        guard !points.isEmpty else { return CGPoint(x: 0.5, y: 0.5) }
        var sumX: CGFloat = 0, sumY: CGFloat = 0
        for p in points { sumX += p.x; sumY += p.y }
        return CGPoint(x: sumX / CGFloat(points.count), y: sumY / CGFloat(points.count))
    }

    private func computeEAR(points: VNFaceLandmarkRegion2D) -> Double {
        let pts = points.normalizedPoints
        guard pts.count >= 6 else { return 0.3 }
        let xs = pts.map { $0.x }
        let ys = pts.map { $0.y }
        let w = Double((xs.max() ?? 1) - (xs.min() ?? 0))
        let h = Double((ys.max() ?? 1) - (ys.min() ?? 0))
        guard w > 0 else { return 0.3 }
        return h / w
    }
}
