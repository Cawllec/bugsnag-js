//
//  BSGEventUploader.h
//  Bugsnag
//
//  Created by Nick Dowell on 16/02/2021.
//  Copyright © 2021 Bugsnag Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

@class BugsnagApiClient;
@class BugsnagConfiguration;
@class BugsnagEvent;
@class BugsnagNotifier;

NS_ASSUME_NONNULL_BEGIN

@interface BSGEventUploader : NSObject

- (instancetype)initWithConfiguration:(BugsnagConfiguration *)configuration notifier:(BugsnagNotifier *)notifier;

- (void)uploadEvent:(BugsnagEvent *)event;

- (void)uploadStoredEvents;

- (void)uploadLatestStoredEvent:(void (^)(void))completionHandler;

@end

NS_ASSUME_NONNULL_END
