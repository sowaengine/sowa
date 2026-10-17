#include "editor/native_titlebar.hpp"
#include "editor/style.hpp"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#import <Cocoa/Cocoa.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#import <objc/runtime.h>

namespace {
char titlebarStyleKey;

NSColor *nativeColor(const ImVec4 &color, bool active) {
	const CGFloat intensity = active ? 1.0 : 0.72;
	return [NSColor colorWithSRGBRed:color.x * intensity
							   green:color.y * intensity
								blue:color.z * intensity
							   alpha:color.w];
}
} // namespace

@interface SowaTitlebarBackground : NSView
@end

@implementation SowaTitlebarBackground

- (NSView *)hitTest:(NSPoint)point {
	// Let AppKit handle dragging, double-clicking and the traffic-light buttons.
	return nil;
}

- (void)viewDidMoveToWindow {
	[super viewDidMoveToWindow];
	NSNotificationCenter *notifications = [NSNotificationCenter defaultCenter];
	[notifications removeObserver:self];
	if (self.window) {
		for (NSString *name in @[
				 NSWindowDidBecomeKeyNotification, NSWindowDidResignKeyNotification,
				 NSWindowDidResizeNotification
			 ]) {
			[notifications addObserver:self
							  selector:@selector(refreshTitlebar:)
								  name:name
								object:self.window];
		}
	}
}

- (void)refreshTitlebar:(NSNotification *)notification {
	self.needsDisplay = YES;
}

- (void)drawRect:(NSRect)dirtyRect {
	using namespace sowa::editor::theme;
	const NSRect titleRect = self.bounds;
	if (NSHeight(titleRect) <= 0) {
		return;
	}
	const bool active = self.window.isKeyWindow;
	const CGFloat locations[] = {0.0, 0.58, 1.0};
	NSGradient *gradient = [[NSGradient alloc] initWithColors:@[
		nativeColor(TitleStart, active), nativeColor(TitleMiddle, active),
		nativeColor(TitleEnd, active)
	]
												  atLocations:locations
												   colorSpace:[NSColorSpace sRGBColorSpace]];
	[gradient drawInRect:titleRect angle:0];
	[nativeColor(TitleDivider, active) setFill];
	NSRectFill(NSMakeRect(0, NSMinY(titleRect), NSWidth(titleRect), 1));
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end

@interface SowaTitlebarStyle : NSObject
@property(nonatomic, strong) SowaTitlebarBackground *background;
@property(nonatomic, strong) NSAppearance *previousAppearance;
@property(nonatomic) BOOL previouslyTransparent;
@end

@implementation SowaTitlebarStyle
@end

namespace sowa::editor {

void SetNativeTitlebarStyle(GLFWwindow *handle, bool enabled) {
	if (!handle) {
		return;
	}
	NSWindow *window = glfwGetCocoaWindow(handle);
	SowaTitlebarStyle *style = objc_getAssociatedObject(window, &titlebarStyleKey);
	if (!enabled) {
		if (style) {
			[style.background removeFromSuperview];
			window.titlebarAppearsTransparent = style.previouslyTransparent;
			window.appearance = style.previousAppearance;
			objc_setAssociatedObject(window, &titlebarStyleKey, nil,
				OBJC_ASSOCIATION_RETAIN_NONATOMIC);
		}
		return;
	}
	NSView *titlebar = [window standardWindowButton:NSWindowCloseButton].superview;
	if (style || !titlebar) {
		return;
	}
	style = [[SowaTitlebarStyle alloc] init];
	style.previousAppearance = window.appearance;
	style.previouslyTransparent = window.titlebarAppearsTransparent;
	// Paint inside the native title bar, behind its controls and title. The
	// content view stays untouched, including GLFW's size and cursor mapping.
	style.background = [[SowaTitlebarBackground alloc] initWithFrame:titlebar.bounds];
	style.background.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
	window.titlebarAppearsTransparent = YES;
	[titlebar addSubview:style.background positioned:NSWindowBelow relativeTo:nil];
	objc_setAssociatedObject(window, &titlebarStyleKey, style, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

} // namespace sowa::editor
