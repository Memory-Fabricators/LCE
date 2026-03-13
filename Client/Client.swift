#if os(macOS)
    import AppKit
#endif

@main
public class Client {
    private var window: Window

    public init() {
        window = Window(title: "Minecraft", width: 800, height: 600)
    }

    public func run() {
        do {
            try window.runWith { renderer in
                #if os(macOS)
                    renderer.render()
                #endif
            }
        } catch {
            print("Error: \(error)\n\(error.localizedDescription)")
        }
    }

    public static func main() {
        let client = Client()
        client.run()
    }
}
