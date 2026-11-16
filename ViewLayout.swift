private struct LyraViewBounds {
    let x: Float
    let y: Float
    let width: Float
    let height: Float

    var right: Float { x + width }
    var bottom: Float { y + height }
}

private struct LyraViewChild {
    let id: UInt64
    let width: Float
    let height: Float
}

private struct LyraViewRecord {
    let orientation: Int32
    let bounds: LyraViewBounds
    var children: [LyraViewChild] = []
}

private final class LyraViewSpaceState {
    var nextID: UInt64 = 1
    var views: [UInt64: LyraViewRecord] = [:]
}

private enum LyraViewStatus {
    static let success: Int32 = 0
    static let invalidBounds: Int32 = 1
    static let overlap: Int32 = 2
    static let componentDoesNotFit: Int32 = 3
    static let componentAlreadyAdded: Int32 = 4
    static let invalidHandle: Int32 = 5
}

private func viewSpaceState(_ opaque: UnsafeMutableRawPointer?) -> LyraViewSpaceState? {
    guard let opaque else { return nil }
    return Unmanaged<LyraViewSpaceState>.fromOpaque(opaque).takeUnretainedValue()
}

private func valid(_ bounds: LyraViewBounds) -> Bool {
    bounds.x.isFinite && bounds.y.isFinite &&
        bounds.width.isFinite && bounds.height.isFinite &&
        bounds.width > 0 && bounds.height > 0 &&
        bounds.right.isFinite && bounds.bottom.isFinite
}

private func intersects(_ lhs: LyraViewBounds, _ rhs: LyraViewBounds) -> Bool {
    lhs.x < rhs.right && lhs.right > rhs.x &&
        lhs.y < rhs.bottom && lhs.bottom > rhs.y
}

private func componentFits(
    _ child: LyraViewChild,
    in view: LyraViewRecord
) -> Bool {
    guard child.width.isFinite && child.height.isFinite &&
            child.width > 0 && child.height > 0 &&
            child.width <= view.bounds.width &&
            child.height <= view.bounds.height else {
        return false
    }

    let used = view.children.reduce(Float.zero) {
        $0 + (view.orientation == 0 ? $1.width : $1.height)
    }
    let available = view.orientation == 0
        ? view.bounds.width : view.bounds.height
    let requested = view.orientation == 0 ? child.width : child.height
    return requested <= available - used
}

@_cdecl("lyraViewSpaceCreate")
public func lyraViewSpaceCreate() -> UnsafeMutableRawPointer {
    Unmanaged.passRetained(LyraViewSpaceState()).toOpaque()
}

@_cdecl("lyraViewSpaceDestroy")
public func lyraViewSpaceDestroy(_ opaque: UnsafeMutableRawPointer?) {
    guard let opaque else { return }
    Unmanaged<LyraViewSpaceState>.fromOpaque(opaque).release()
}

@_cdecl("lyraViewCreate")
public func lyraViewCreate(
    _ opaque: UnsafeMutableRawPointer?,
    _ orientation: Int32,
    _ x: Float,
    _ y: Float,
    _ width: Float,
    _ height: Float,
    _ viewID: UnsafeMutablePointer<UInt64>?
) -> Int32 {
    guard let state = viewSpaceState(opaque), let viewID,
          orientation == 0 || orientation == 1 else {
        return LyraViewStatus.invalidHandle
    }

    let bounds = LyraViewBounds(x: x, y: y, width: width, height: height)
    guard valid(bounds) else { return LyraViewStatus.invalidBounds }
    if state.views.values.contains(where: { intersects(bounds, $0.bounds) }) {
        return LyraViewStatus.overlap
    }

    let id = state.nextID
    state.nextID &+= 1
    state.views[id] = LyraViewRecord(orientation: orientation, bounds: bounds)
    viewID.pointee = id
    return LyraViewStatus.success
}

@_cdecl("lyraViewRemove")
public func lyraViewRemove(
    _ opaque: UnsafeMutableRawPointer?,
    _ viewID: UInt64
) -> Int32 {
    guard let state = viewSpaceState(opaque),
          state.views.removeValue(forKey: viewID) != nil else {
        return LyraViewStatus.invalidHandle
    }
    return LyraViewStatus.success
}

@_cdecl("lyraViewAddComponent")
public func lyraViewAddComponent(
    _ opaque: UnsafeMutableRawPointer?,
    _ viewID: UInt64,
    _ componentID: UInt64,
    _ width: Float,
    _ height: Float
) -> Int32 {
    guard let state = viewSpaceState(opaque),
          var view = state.views[viewID] else {
        return LyraViewStatus.invalidHandle
    }
    guard !view.children.contains(where: { $0.id == componentID }) else {
        return LyraViewStatus.componentAlreadyAdded
    }

    let child = LyraViewChild(id: componentID, width: width, height: height)
    guard componentFits(child, in: view) else {
        return LyraViewStatus.componentDoesNotFit
    }
    view.children.append(child)
    state.views[viewID] = view
    return LyraViewStatus.success
}

@_cdecl("lyraViewRemoveComponent")
public func lyraViewRemoveComponent(
    _ opaque: UnsafeMutableRawPointer?,
    _ viewID: UInt64,
    _ componentID: UInt64
) -> Int32 {
    guard let state = viewSpaceState(opaque),
          var view = state.views[viewID],
          let index = view.children.firstIndex(where: { $0.id == componentID }) else {
        return LyraViewStatus.invalidHandle
    }
    view.children.remove(at: index)
    state.views[viewID] = view
    return LyraViewStatus.success
}

@_cdecl("lyraViewGetComponentPosition")
public func lyraViewGetComponentPosition(
    _ opaque: UnsafeMutableRawPointer?,
    _ viewID: UInt64,
    _ componentID: UInt64,
    _ x: UnsafeMutablePointer<Float>?,
    _ y: UnsafeMutablePointer<Float>?
) -> Int32 {
    guard let state = viewSpaceState(opaque),
          let view = state.views[viewID],
          let index = view.children.firstIndex(where: { $0.id == componentID }),
          let x, let y else {
        return LyraViewStatus.invalidHandle
    }

    var positionX = view.bounds.x
    var positionY = view.bounds.y
    for child in view.children[..<index] {
        if view.orientation == 0 {
            positionX += child.width
        } else {
            positionY += child.height
        }
    }
    x.pointee = positionX
    y.pointee = positionY
    return LyraViewStatus.success
}
