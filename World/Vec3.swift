public struct Vec3 {
    private var data: SIMD4<Float>

    public init(x: Float, y: Float, z: Float) {
        self.data = SIMD4<Float>(x, y, z, 0)
    }

    public init(data: SIMD4<Float>) {
        self.data = data
    }

    public var x: Float {
        get { return data.x }
        set { data.x = newValue }
    }

    public var y: Float {
        get { return data.y }
        set { data.y = newValue }
    }

    public var z: Float {
        get { return data.z }
        set { data.z = newValue }
    }

    public func interpolateTo(t: Vec3, p: Float) -> Vec3 {
        let x = x + (t.x - x) * p
        let y = y + (t.y - y) * p
        let z = z + (t.z - z) * p
        return Vec3(x: x, y: y, z: z)
    }

    public static func + (left: Vec3, right: Vec3) -> Vec3 {
        Vec3(data: left.data + right.data)
    }

    public static func - (left: Vec3, right: Vec3) -> Vec3 {
        Vec3(data: left.data - right.data)
    }
}
