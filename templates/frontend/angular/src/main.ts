import { provideZonelessChangeDetection } from "@angular/core";
import { bootstrapApplication } from "@angular/platform-browser";
import { App } from "./app/app";

// signals tell Angular what changed, so it doesn't need zone.js
bootstrapApplication(App, { providers: [provideZonelessChangeDetection()] }).catch((err) => console.error(err));
