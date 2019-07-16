#import <Foundation/Foundation.h>
#import <AppKit/NSWorkspace.h>
#import <AppKit/NSRunningApplication.h>
#import <AppKit/NSApplication.h>
#import <AppKit/NSImage.h>



#include "common.h"

bool activate_window(char *name) {
    BOOL activationResult = 0;
    NSString *appName = [NSString stringWithUTF8String:name];

    NSArray *apps = [NSWorkspace sharedWorkspace].runningApplications;
    for (NSRunningApplication *app in apps) {
        if ([app.localizedName isEqual: appName]) {
            activationResult = [app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
        }
    }
    return activationResult;
}

struct DesktopWindow* list_windows(size_t *count) {
    // Use AppleScript since the only way to read window titles
    // For this to work, you need to give permissions in Settings > Privacy & Security > Accessibility

    // example https://stackoverflow.com/a/6805256
    // TODO read from compiled script (app_titles.scpt)
    NSMutableString *scriptText = [NSMutableString stringWithString:@"global apps\n"];
    [scriptText appendString:@"set apps to {}\n"];
    [scriptText appendString:@"tell application \"System Events\"\n"];
    [scriptText appendString:@"set processList to every process whose visible is true\n"];
    [scriptText appendString:@"repeat with a from 1 to length of processList\n"];
    [scriptText appendString:@"set proc to item a of processList\n"];
    [scriptText appendString:@"set appName to name of proc\n"];
    [scriptText appendString:@"tell process appName\n"];
    [scriptText appendString:@"try\n"];
    [scriptText appendString:@"tell (1st window whose value of attribute \"AXMain\" is true)\n"];
    [scriptText appendString:@"copy {name:appName, title:value of attribute \"AXTitle\"} to the end of apps\n"];
    [scriptText appendString:@"end tell\n"];
    [scriptText appendString:@"end try\n"];
    [scriptText appendString:@"end tell\n"];
    [scriptText appendString:@"end repeat\n"];
    [scriptText appendString:@"end tell\n"];
    [scriptText appendString:@"return apps"];

    NSDictionary *error = nil;

    NSAppleScript *script = [[[NSAppleScript alloc] initWithSource:scriptText] autorelease];

    NSAppleEventDescriptor *apps = [script executeAndReturnError:&error];

    const NSInteger list_count = [apps numberOfItems];

    struct DesktopWindow *window_list = (struct DesktopWindow*)malloc(list_count * sizeof(struct DesktopWindow));

    for (int i = 0; i < list_count; i++) {
        NSAppleEventDescriptor *app = [apps descriptorAtIndex:i+1];

        const NSString *appName = [[app descriptorForKeyword:[app keywordForDescriptorAtIndex:1]] stringValue];
        NSUInteger appNameLen = [appName lengthOfBytesUsingEncoding:NSUTF8StringEncoding];

        const NSString *appTitle = [[app descriptorForKeyword:[app keywordForDescriptorAtIndex:2]] stringValue];
        NSInteger appTitleLen = [appTitle lengthOfBytesUsingEncoding:NSUTF8StringEncoding];

        if (appNameLen > WINDOW_ID_LEN)
        {
            NSLog(@"[WARN] App name exceeded %d bytes and was truncated.", WINDOW_ID_LEN);
            appNameLen = WINDOW_ID_LEN;
        }

        if (appTitleLen > WINDOW_TITLE_LEN)
        {
            NSLog(@"[WARN] App name exceeded %d bytes and was truncated.", WINDOW_ID_LEN);
            appTitleLen = WINDOW_TITLE_LEN;
        }

        strcpy(window_list[i].id, [appName UTF8String]);
        strcpy(window_list[i].title, [appTitle UTF8String]);
        window_list[i].id_size = appNameLen;
        window_list[i].title_size = appTitleLen;
    }

    *count = list_count;
    return window_list;
}

struct DesktopApplication** list_applications(size_t *count) {
    int i = 0;

    // https://stackoverflow.com/a/22112941
    __block BOOL finish = NO;

    //https://stackoverflow.com/a/24281487
    NSMetadataQuery *query = [NSMetadataQuery new];
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"kMDItemKind == 'Application'"];
    [[NSNotificationCenter defaultCenter] addObserverForName:
                    NSMetadataQueryDidFinishGatheringNotification
                                                      object:nil queue:[NSOperationQueue new]
                                                  usingBlock:^(NSNotification __strong *notification)
                                                  {
                                                      finish = YES;
                                                  }
    ];
    [query setPredicate:predicate];
    [query startQuery];

    // TODO Handle this with promises
    // https://stackoverflow.com/a/36732342
    // https://stackoverflow.com/q/22112700
    while(!finish) {
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    }

    const NSUInteger list_count = [query.results count];
    struct DesktopApplication **apps = (struct DesktopApplication**)malloc(list_count * sizeof(struct DesktopApplication *));

    for (NSMetadataItem *item in query.results) {
        NSString *appId = [item valueForAttribute:NSMetadataItemCFBundleIdentifierKey];
        if (!appId) {
            appId = @"";
        }
        const char *appIdUTF8 = [appId cStringUsingEncoding:NSUTF8StringEncoding];
        NSUInteger appIdLen = [appId lengthOfBytesUsingEncoding:NSUTF8StringEncoding];

        /* start get app name */
        NSString *appName = [item valueForAttribute:NSMetadataItemDisplayNameKey];
        const char *appNameUTF8 = [appName cStringUsingEncoding:NSUTF8StringEncoding];
        NSUInteger appNameLen = [appName lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        /* end get app name */

        /* start get app path */
        NSString *bundlePath = [item valueForAttribute:NSMetadataItemPathKey];
        const char *bundlePathUTF8 = [bundlePath cStringUsingEncoding:NSUTF8StringEncoding];
        NSUInteger bundlePathLen = [bundlePath lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        /* end get app path */


        /* Get app icon as base64 string */
        BOOL hasIcon = 0;
        if ([bundlePath length] > 0) {
            NSBundle *bundle = [NSBundle bundleWithPath:bundlePath];
            NSDictionary<NSString *, id> *infos = bundle.infoDictionary;
            NSImageName iconName = infos[@"CFBundleIconFile"];
            NSString *iconPath = [bundle pathForImageResource:iconName];

            if ([iconPath length] > 0) {
                NSRect rect = NSMakeRect(0, 0, ICON_SIZE, ICON_SIZE);

                NSImage *iconImg = NULL;
                if ([[iconPath pathExtension] isEqualToString:@"icns"]) {
                    iconImg = [[NSImage alloc] initWithContentsOfFile:iconPath];
                } else {
                    iconImg = [[NSWorkspace sharedWorkspace] iconForFile:iconPath];
                }

                NSImageRep *iconRep = [iconImg bestRepresentationForRect:rect context:NULL hints:NULL];
                NSImage *bestIcon = [[NSImage alloc] init];
                [bestIcon addRepresentation:iconRep];

                CGImageRef CGImage = [bestIcon CGImageForProposedRect:nil context:nil hints:nil];
                NSBitmapImageRep *bitmap = [[[NSBitmapImageRep alloc] initWithCGImage:CGImage] autorelease];
                NSData *png = [bitmap representationUsingType:NSPNGFileType properties:[NSMutableDictionary dictionary]];
                NSString *base64 = [@"data:image/png;base64," stringByAppendingString:[png base64EncodedStringWithOptions:0]];

                [bestIcon release];
                [iconImg release];

                const char *base64UTF8 = [base64 UTF8String];
                const NSUInteger base64UTF8Len = [base64 lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
                apps[i] = (struct DesktopApplication*)malloc(sizeof(struct DesktopApplication) + sizeof(char) * base64UTF8Len);
                strcpy(apps[i]->icon, base64UTF8);
                apps[i]->icon_size = base64UTF8Len;

                hasIcon = 1;
            }
        }

        if (!hasIcon) {
            apps[i] = (struct DesktopApplication*)malloc(sizeof(struct DesktopApplication));
            apps[i]->icon_size = 0;
        }
        /* end get app icon */

        strcpy(apps[i]->id, appIdUTF8);
        strcpy(apps[i]->name, appNameUTF8);
        strcpy(apps[i]->path, bundlePathUTF8);
        apps[i]->id_size = appIdLen;
        apps[i]->path_size = bundlePathLen;
        apps[i]->name_size = appNameLen;

        i++;
    }

    *count = (size_t)list_count;
    return apps;
}