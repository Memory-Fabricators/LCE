#if os(macOS)
    import AppKit
#endif

public class Window {
    #if os(macOS)
        private var window: NSWindow
        private var contentView: NSView
        let metalLayer: CAMetalLayer
    #endif

    public init(title: String, width: Int, height: Int) {
        #if os(macOS)
            let rect = NSRect(x: 0, y: 0, width: width, height: height)
            window = NSWindow(
                contentRect: rect,
                styleMask: [.titled, .closable, .resizable, .miniaturizable],
                backing: .buffered,
                defer: false
            )

            window.title = title

            contentView = NSView(frame: rect)
            window.contentView = contentView

            metalLayer = CAMetalLayer()
            metalLayer.device = MTLCreateSystemDefaultDevice()
            contentView.wantsLayer = true
            contentView.layer = metalLayer

            window.makeKeyAndOrderFront(nil)
        #endif
    }

    public func runWith(callback: (Renderer) throws -> Void) throws {
        #if os(macOS)
            let app = NSApplication.shared
            app.setActivationPolicy(.regular)
            app.activate(ignoringOtherApps: true)

            app.finishLaunching()  // Essential when manually running the loop

            var shouldQuit = false
            let renderer = try Renderer(metalLayer: metalLayer)

            while !shouldQuit {
                while let event = app.nextEvent(
                    matching: .any, until: nil, inMode: .default, dequeue: true)
                {
                    if event.type == .appKitDefined && event.subtype.rawValue == 15 {  // Handle Quit
                        shouldQuit = true
                    }
                    app.sendEvent(event)
                }

                try callback(renderer)
            }
        #endif
    }
}
