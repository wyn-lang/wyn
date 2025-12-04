// Wyn Mobile Runtime - iOS (UIKit)
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import <objc/message.h>

// Global app delegate
static id g_app_delegate = NULL;
static id g_window = NULL;
static id g_root_view_controller = NULL;

// Callback storage
typedef struct {
    void (*callback)(void);
} CallbackData;

// AppDelegate implementation
@interface WynAppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation WynAppDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    UIViewController *rootVC = [[UIViewController alloc] init];
    rootVC.view.backgroundColor = [UIColor whiteColor];
    self.window.rootViewController = rootVC;
    [self.window makeKeyAndVisible];
    g_window = (__bridge id)self.window;
    g_root_view_controller = (__bridge id)rootVC;
    return YES;
}
@end

// Button action handler
@interface WynButtonTarget : NSObject
@property (nonatomic) CallbackData *callbackData;
- (void)buttonTapped:(id)sender;
@end

@implementation WynButtonTarget
- (void)buttonTapped:(id)sender {
    if (self.callbackData && self.callbackData->callback) {
        self.callbackData->callback();
    }
}
@end

// TextField delegate
@interface WynTextFieldDelegate : NSObject <UITextFieldDelegate>
@property (nonatomic) CallbackData *callbackData;
@end

@implementation WynTextFieldDelegate
- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    if (self.callbackData && self.callbackData->callback) {
        dispatch_async(dispatch_get_main_queue(), ^{
            self.callbackData->callback();
        });
    }
    return YES;
}
@end

// Initialize UIApplication
void mobile_init() {
    if (g_app_delegate) return;
    
    @autoreleasepool {
        g_app_delegate = [[WynAppDelegate alloc] init];
    }
}

// Start event loop
void mobile_run() {
    @autoreleasepool {
        UIApplicationMain(0, NULL, nil, NSStringFromClass([WynAppDelegate class]));
    }
}

// Create view
int64_t mobile_create_view(int64_t x, int64_t y, int64_t w, int64_t h) {
    @autoreleasepool {
        UIView *view = [[UIView alloc] initWithFrame:CGRectMake(x, y, w, h)];
        view.backgroundColor = [UIColor whiteColor];
        return (int64_t)(__bridge_retained void *)view;
    }
}

// Add subview
void mobile_add_subview(int64_t parent_handle, int64_t child_handle) {
    @autoreleasepool {
        UIView *parent = (__bridge UIView *)(void *)parent_handle;
        UIView *child = (__bridge UIView *)(void *)child_handle;
        [parent addSubview:child];
    }
}

// Add to root view
void mobile_add_to_root(int64_t view_handle) {
    @autoreleasepool {
        UIView *view = (__bridge UIView *)(void *)view_handle;
        UIViewController *rootVC = (__bridge UIViewController *)g_root_view_controller;
        [rootVC.view addSubview:view];
    }
}

// Create button
int64_t mobile_create_button(const char* title, int64_t x, int64_t y, int64_t w, int64_t h) {
    @autoreleasepool {
        UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
        button.frame = CGRectMake(x, y, w, h);
        [button setTitle:[NSString stringWithUTF8String:title] forState:UIControlStateNormal];
        button.backgroundColor = [UIColor systemBlueColor];
        [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        button.layer.cornerRadius = 8;
        return (int64_t)(__bridge_retained void *)button;
    }
}

// Create label
int64_t mobile_create_label(const char* text, int64_t x, int64_t y, int64_t w, int64_t h) {
    @autoreleasepool {
        UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(x, y, w, h)];
        label.text = [NSString stringWithUTF8String:text];
        label.textAlignment = NSTextAlignmentCenter;
        label.font = [UIFont systemFontOfSize:17];
        return (int64_t)(__bridge_retained void *)label;
    }
}

// Create text field
int64_t mobile_create_textfield(int64_t x, int64_t y, int64_t w, int64_t h) {
    @autoreleasepool {
        UITextField *textField = [[UITextField alloc] initWithFrame:CGRectMake(x, y, w, h)];
        textField.borderStyle = UITextBorderStyleRoundedRect;
        textField.font = [UIFont systemFontOfSize:17];
        textField.placeholder = @"Enter text";
        return (int64_t)(__bridge_retained void *)textField;
    }
}

// Set text
void mobile_set_text(int64_t view_handle, const char* text) {
    @autoreleasepool {
        id view = (__bridge id)(void *)view_handle;
        NSString *str = [NSString stringWithUTF8String:text];
        
        if ([view isKindOfClass:[UILabel class]]) {
            [(UILabel *)view setText:str];
        } else if ([view isKindOfClass:[UITextField class]]) {
            [(UITextField *)view setText:str];
        } else if ([view isKindOfClass:[UIButton class]]) {
            [(UIButton *)view setTitle:str forState:UIControlStateNormal];
        }
    }
}

// Get text
char* mobile_get_text(int64_t view_handle) {
    @autoreleasepool {
        id view = (__bridge id)(void *)view_handle;
        NSString *text = nil;
        
        if ([view isKindOfClass:[UILabel class]]) {
            text = [(UILabel *)view text];
        } else if ([view isKindOfClass:[UITextField class]]) {
            text = [(UITextField *)view text];
        } else if ([view isKindOfClass:[UIButton class]]) {
            text = [(UIButton *)view titleForState:UIControlStateNormal];
        }
        
        if (text) {
            const char *cstr = [text UTF8String];
            char *result = malloc(strlen(cstr) + 1);
            strcpy(result, cstr);
            return result;
        }
        return NULL;
    }
}

// Set frame
void mobile_set_frame(int64_t view_handle, int64_t x, int64_t y, int64_t w, int64_t h) {
    @autoreleasepool {
        UIView *view = (__bridge UIView *)(void *)view_handle;
        view.frame = CGRectMake(x, y, w, h);
    }
}

// Set background color
void mobile_set_background_color(int64_t view_handle, int64_t r, int64_t g, int64_t b) {
    @autoreleasepool {
        UIView *view = (__bridge UIView *)(void *)view_handle;
        view.backgroundColor = [UIColor colorWithRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
    }
}

// Set text color
void mobile_set_text_color(int64_t view_handle, int64_t r, int64_t g, int64_t b) {
    @autoreleasepool {
        id view = (__bridge id)(void *)view_handle;
        UIColor *color = [UIColor colorWithRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
        
        if ([view isKindOfClass:[UILabel class]]) {
            [(UILabel *)view setTextColor:color];
        } else if ([view isKindOfClass:[UIButton class]]) {
            [(UIButton *)view setTitleColor:color forState:UIControlStateNormal];
        } else if ([view isKindOfClass:[UITextField class]]) {
            [(UITextField *)view setTextColor:color];
        }
    }
}

// Set font size
void mobile_set_font_size(int64_t view_handle, int64_t size) {
    @autoreleasepool {
        id view = (__bridge id)(void *)view_handle;
        UIFont *font = [UIFont systemFontOfSize:size];
        
        if ([view isKindOfClass:[UILabel class]]) {
            [(UILabel *)view setFont:font];
        } else if ([view isKindOfClass:[UIButton class]]) {
            [(UIButton *)view.titleLabel setFont:font];
        } else if ([view isKindOfClass:[UITextField class]]) {
            [(UITextField *)view setFont:font];
        }
    }
}

// On tap callback
void mobile_on_tap(int64_t view_handle, void (*callback)(void)) {
    @autoreleasepool {
        id view = (__bridge id)(void *)view_handle;
        
        if ([view isKindOfClass:[UIButton class]]) {
            UIButton *button = (UIButton *)view;
            
            CallbackData *data = malloc(sizeof(CallbackData));
            data->callback = callback;
            
            WynButtonTarget *target = [[WynButtonTarget alloc] init];
            target.callbackData = data;
            
            objc_setAssociatedObject(button, "target", target, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            objc_setAssociatedObject(button, "callback", [NSValue valueWithPointer:data], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            
            [button addTarget:target action:@selector(buttonTapped:) forControlEvents:UIControlEventTouchUpInside];
        } else if ([view isKindOfClass:[UIView class]]) {
            UIView *uiView = (UIView *)view;
            
            CallbackData *data = malloc(sizeof(CallbackData));
            data->callback = callback;
            
            WynButtonTarget *target = [[WynButtonTarget alloc] init];
            target.callbackData = data;
            
            objc_setAssociatedObject(uiView, "target", target, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            objc_setAssociatedObject(uiView, "callback", [NSValue valueWithPointer:data], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            
            UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:target action:@selector(buttonTapped:)];
            [uiView addGestureRecognizer:tap];
            uiView.userInteractionEnabled = YES;
        }
    }
}

// On text change callback
void mobile_on_text_change(int64_t textfield_handle, void (*callback)(void)) {
    @autoreleasepool {
        UITextField *textField = (__bridge UITextField *)(void *)textfield_handle;
        
        CallbackData *data = malloc(sizeof(CallbackData));
        data->callback = callback;
        
        WynTextFieldDelegate *delegate = [[WynTextFieldDelegate alloc] init];
        delegate.callbackData = data;
        
        objc_setAssociatedObject(textField, "delegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(textField, "callback", [NSValue valueWithPointer:data], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        
        textField.delegate = delegate;
    }
}

// Show alert
void mobile_show_alert(const char* title, const char* message) {
    @autoreleasepool {
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:title]
                                                                       message:[NSString stringWithUTF8String:message]
                                                                preferredStyle:UIAlertControllerStyleAlert];
        
        UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil];
        [alert addAction:okAction];
        
        UIViewController *rootVC = (__bridge UIViewController *)g_root_view_controller;
        [rootVC presentViewController:alert animated:YES completion:nil];
    }
}

// Hide view
void mobile_hide_view(int64_t view_handle) {
    @autoreleasepool {
        UIView *view = (__bridge UIView *)(void *)view_handle;
        view.hidden = YES;
    }
}

// Show view
void mobile_show_view(int64_t view_handle) {
    @autoreleasepool {
        UIView *view = (__bridge UIView *)(void *)view_handle;
        view.hidden = NO;
    }
}

// Remove view
void mobile_remove_view(int64_t view_handle) {
    @autoreleasepool {
        UIView *view = (__bridge UIView *)(void *)view_handle;
        [view removeFromSuperview];
        CFRelease((void *)view_handle);
    }
}
