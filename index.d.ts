declare module "desktop-native" {
    export interface Window {
        identifier: string;
        title: string;
    }

    export interface Application {
        name: string;
        icon: string;
        path: string;
        id: string;
    }

    export function activateWindow(name: string): boolean;
    export function listWindows(): Window[];
    export function listApplications(): Application[];
}