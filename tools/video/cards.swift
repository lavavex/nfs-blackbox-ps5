// Style A cards (the owner's pick): centred, thin and widely spaced Futura, a short amber rule,
// a light dim behind; transparent 1920x1080 PNGs.
import AppKit

let W: CGFloat = 1920, H: CGFloat = 1080
let amber = NSColor(red: 0.98, green: 0.69, blue: 0.10, alpha: 1)
func f(_ n: String, _ s: CGFloat) -> NSFont { NSFont(name: n, size: s) ?? .boldSystemFont(ofSize: s) }

func save(_ img: NSImage, _ out: String) {
    let rep = NSBitmapImageRep(data: img.tiffRepresentation!)!
    try! rep.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: out))
}
func text(_ s: String, _ font: NSFont, _ color: NSColor, _ kern: CGFloat, centreX: CGFloat, y: CGFloat) {
    let sh = NSShadow(); sh.shadowColor = NSColor(white: 0, alpha: 0.9); sh.shadowBlurRadius = 20; sh.shadowOffset = .zero
    let a = NSAttributedString(string: s, attributes: [.font: font, .foregroundColor: color, .kern: kern, .shadow: sh])
    a.draw(at: NSPoint(x: centreX - a.size().width / 2, y: y))
}

// Small line, big line, amber rule, amber line (any may be empty), centred on the frame.
func card(_ out: String, top: String, big: String, bottom: String, bigSize: CGFloat = 150, extra: String = "", dim: CGFloat = 0.35) {
    let img = NSImage(size: NSSize(width: W, height: H)); img.lockFocus()
    NSColor(white: 0, alpha: dim).set(); NSRect(x: 0, y: 0, width: W, height: H).fill(using: .sourceOver)
    if !top.isEmpty { text(top, f("Futura-Medium", 46), .white, 22, centreX: W / 2, y: 600) }
    text(big, f("Futura-Bold", bigSize), .white, 14, centreX: W / 2, y: 420)
    amber.set(); NSRect(x: W / 2 - 60, y: 400, width: 120, height: 4).fill()
    if !bottom.isEmpty { text(bottom, f("Futura-Medium", 40), amber, 18, centreX: W / 2, y: 330) }
    if !extra.isEmpty { text(extra, f("Futura-Medium", 32), NSColor(white: 1, alpha: 0.85), 6, centreX: W / 2, y: 250) }
    img.unlockFocus(); save(img, out)
}

// A low card for a shot with its own UI in the middle: two lines at the bottom over a dim that
// rises from the bottom edge only.
// Left-aligned at the bottom, clear of the launcher's panel on the right (x 1336 on).
func lowCard(_ out: String, top: String, big: String) {
    let img = NSImage(size: NSSize(width: W, height: H)); img.lockFocus()
    NSGradient(colors: [NSColor(white: 0, alpha: 0.75), NSColor(white: 0, alpha: 0)])!
        .draw(in: NSRect(x: 0, y: 0, width: W, height: 300), angle: 90)
    let sh = NSShadow(); sh.shadowColor = NSColor(white: 0, alpha: 0.9); sh.shadowBlurRadius = 20; sh.shadowOffset = .zero
    for (s, font, color, kern, y) in [(top, f("Futura-Medium", 34), amber, CGFloat(14), CGFloat(170)),
                                     (big, f("Futura-Bold", 84), NSColor.white, CGFloat(10), CGFloat(66))] {
        NSAttributedString(string: s, attributes: [.font: font, .foregroundColor: color, .kern: kern, .shadow: sh])
            .draw(at: NSPoint(x: 96, y: y))
    }
    amber.set(); NSRect(x: 96, y: 54, width: 120, height: 4).fill()
    img.unlockFocus(); save(img, out)
}

// The comparison's labels: one over each half, the same lettering, no dim.
func labels(_ out: String) {
    let img = NSImage(size: NSSize(width: W, height: H)); img.lockFocus()
    NSColor(white: 0, alpha: 0.8).set(); NSRect(x: W / 2 - 2, y: 0, width: 4, height: H).fill()
    text("XBOX 360 LOOK", f("Futura-Bold", 54), amber, 10, centreX: W / 4, y: H - 130)
    text("PC ORIGINAL", f("Futura-Bold", 54), .white, 10, centreX: 3 * W / 4, y: H - 130)
    img.unlockFocus(); save(img, out)
}

// The slider's labels: the same lettering, no line (the divider moves).
func sliderLabels(_ out: String) {
    let img = NSImage(size: NSSize(width: W, height: H)); img.lockFocus()
    text("XBOX 360 LOOK", f("Futura-Bold", 50), amber, 10, centreX: 300, y: H - 120)
    text("ORIGINAL", f("Futura-Bold", 50), .white, 10, centreX: W - 230, y: H - 120)
    img.unlockFocus(); save(img, out)
}

let d = CommandLine.arguments[1]
sliderLabels("\(d)/a_slider.png")
card("\(d)/a_title.png", top: "NEED FOR SPEED", big: "MOST WANTED", bottom: "NOW ON PS5")
card("\(d)/a_options.png", top: "IN THE GAME'S OWN MENU", big: "PS5 OPTIONS", bottom: "OVERLAY · 360 LOOK · REPAIR")
lowCard("\(d)/a_install.png", top: "ABOUT THREE MINUTES", big: "ONE PRESS INSTALL")
card("\(d)/a_end.png", top: "NOW ON", big: "PROSPERO STORE", bottom: "BRING YOUR OWN COPY", bigSize: 128,
     extra: "github.com/lavavex/nfs-blackbox-ps5", dim: 0.5)
labels("\(d)/a_labels.png")
print("style A cards written")
