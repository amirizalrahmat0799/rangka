import { describe, expect, it } from "vitest";
import { remaining, titleError } from "./todos";

describe("titleError", () => {
  it("accepts a normal title", () => {
    expect(titleError("Buy milk")).toBeNull();
  });

  it("refuses blank titles", () => {
    expect(titleError("")).not.toBeNull();
    expect(titleError("   ")).not.toBeNull();
  });

  it("refuses titles over 200 characters", () => {
    expect(titleError("x".repeat(200))).toBeNull();
    expect(titleError("x".repeat(201))).not.toBeNull();
  });
});

describe("remaining", () => {
  it("counts todos that aren't done", () => {
    expect(
      remaining([
        { id: 1, title: "a", done: true },
        { id: 2, title: "b", done: false },
        { id: 3, title: "c", done: false },
      ]),
    ).toBe(2);
  });
});
