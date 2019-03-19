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
    uint i = 0;
    NSArray *apps = [NSWorkspace sharedWorkspace].runningApplications;

    const unsigned long list_count = [apps count];
    struct DesktopWindow *window_list = (struct DesktopWindow*)malloc(list_count * sizeof(struct DesktopWindow));

    for (NSRunningApplication *app in apps) {
        const char *appName = [app.localizedName cStringUsingEncoding:NSUTF8StringEncoding];
        NSUInteger appNameLen = [app.localizedName lengthOfBytesUsingEncoding:NSUTF8StringEncoding];

        strcpy(window_list[i].id, appName);
        strcpy(window_list[i].title, appName);
        window_list[i].id_size = appNameLen;
        window_list[i].title_size = appNameLen;

        i++;
    }

    *count = list_count;
    return window_list;
}

struct DesktopApplication** list_applications(size_t *count) {
    int i = 0;

    // https://stackoverflow.com/a/22112941
    __block BOOL finish = NO;

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
    while(!finish) {
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    }

    const NSUInteger list_count = [query.results count];
    struct DesktopApplication **apps = (struct DesktopApplication**)malloc(list_count * sizeof(struct DesktopApplication *));

    for (NSMetadataItem *item in query.results) {
        /* start get app name */
        NSString *appName = [item valueForAttribute:NSMetadataItemDisplayNameKey];
        const char *appNameUTF8 = [appName cStringUsingEncoding:NSUTF8StringEncoding];
        NSUInteger appNameLen = [appName lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        /* end get app name */

        /* start get app path */
        NSString *bundlePath = [item valueForAttribute:NSMetadataItemPathKey]; //NSMetadataItemCFBundleIdentifierKey];
        const char *bundlePathUTF8 = [bundlePath cStringUsingEncoding:NSUTF8StringEncoding];
        NSUInteger bundlePathLen = [bundlePath lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        /* end get app path */


        /* Get app icon as base64 string */
        if ([bundlePath length] > 0) {
//            NSString *bundlePath = @"/Applications/Google Chrome.app";
            NSBundle *bundle = [NSBundle bundleWithPath:bundlePath];
            NSDictionary<NSString *, id> *infos = bundle.infoDictionary;
            NSImageName iconName = infos[@"CFBundleIconFile"];
            NSString *iconPath = [bundle pathForImageResource:iconName];

            if ([iconPath length] == 0) {
                continue;
            }

            NSRect rect = NSMakeRect(0, 0, 128, 128);

            NSImage *iconImg = NULL;
            if ([[iconPath pathExtension] isEqualToString:@"icns"]) {
                iconImg = [[NSImage alloc] initWithContentsOfFile:iconPath];
            } else {
                iconImg = [[NSWorkspace sharedWorkspace] iconForFile:iconPath];
            }

            NSImageRep *iconRep = [iconImg bestRepresentationForRect:rect context:NULL hints:NULL];
            NSImage *bestIcon = [[NSImage alloc] init];
            [bestIcon addRepresentation:iconRep];
            NSBitmapImageRep *bitmap = [NSBitmapImageRep imageRepsWithData:bestIcon.TIFFRepresentation][0];
            NSData *png = [bitmap representationUsingType:NSPNGFileType properties:NULL];
            NSString *base64 = [@"data:image/png;base64," stringByAppendingString:[png base64EncodedStringWithOptions:0]];

            [bestIcon release];
            [iconImg release];

            const char *base64UTF8 = [base64 UTF8String];
            const NSUInteger base64UTF8Len = [base64 lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
            apps[i] = (struct DesktopApplication*)malloc(sizeof(struct DesktopApplication) + sizeof(char) * base64UTF8Len);
            strcpy(apps[i]->icon, base64UTF8);
            apps[i]->icon_size = base64UTF8Len;

        } else {
            apps[i] = (struct DesktopApplication*)malloc(sizeof(struct DesktopApplication));
            apps[i]->icon_size = 0;
        }
        /* end get app icon */

        strcpy(apps[i]->name, appNameUTF8);
        apps[i]->name_size = appNameLen;
        strcpy(apps[i]->path, bundlePathUTF8);
        apps[i]->path_size = bundlePathLen;

        i++;
    }

    *count = (size_t)list_count;
    return apps;
}