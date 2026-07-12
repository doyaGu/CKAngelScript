# Messaging

The `Message` namespace connects runtime entries and AngelScript Components through `ScriptMessage` and the typed `ScriptMessageTarget` value.

## Targets and Topics

Use topics for event names, and targets for directed messages.

| Target value | Meaning |
| --- | --- |
| No target argument | Broadcast/publish according to topic subscriptions. |
| `Message::Runtime("script-id")` | A runtime entry instance. |
| `Message::Component(ctx, behaviorId)` | The current generation of an AngelScript Component behavior. |

`ScriptMessageTarget` is a copied value. `IsValid()`, `Kind()`, and `ToString()` are available for validation and diagnostics. Component targets capture the component state's generation; deleting and recreating a component with the same `CK_ID` makes the old target stale.

The former string-target `Publish`, `Send`, and `Request` overloads are removed. Construct a target once and pass the value to directed calls. `ScriptMessage.Target()` now returns `ScriptMessageTarget`; use `msg.Target().ToString()` when text is needed for logging.

Topic names should be stable strings such as `game.ready`, `ui.changed`, or `clock.now`.

## Registering Interest

Runtime entries can declare topics in metadata:

```angelscript
[script id="hud.runtime" entry="runtime.as"]
[script.messages topics="game.ready;ui.changed"]
```

Runtime entries and components can also subscribe dynamically:

```angelscript
Message::Subscribe(ctx, "game.ready");
Message::Unsubscribe(ctx, "game.ready");
```

## Publish and Send

`Publish` sends to subscribers. `Send` sends to one explicit target.

```angelscript
dictionary payload;
payload["scene"] = "Menu";

Message::Publish(ctx, "game.ready", payload);
ScriptMessageTarget hud = Message::Runtime("hud.runtime");
Message::Send(ctx, hud, "ui.changed", payload);
```

Both `ScriptContext` and `CKBehaviorContext` overloads are registered:

```angelscript
ScriptMessageTarget Runtime(const string &in id)
ScriptMessageTarget Component(const ScriptContext &in ctx, CK_ID id)
ScriptMessageTarget Component(const CKBehaviorContext &in ctx, CK_ID id)
bool Publish(const ScriptContext &in ctx, const string &in topic, dictionary@ payload = null)
bool Publish(const CKBehaviorContext &in ctx, const string &in topic, dictionary@ payload = null)
bool Publish(const ScriptContext &in ctx, const ScriptMessageTarget &in target, const string &in topic, dictionary@ payload = null)
bool Publish(const CKBehaviorContext &in ctx, const ScriptMessageTarget &in target, const string &in topic, dictionary@ payload = null)
bool Send(const ScriptContext &in ctx, const ScriptMessageTarget &in target, const string &in topic, dictionary@ payload = null)
bool Send(const CKBehaviorContext &in ctx, const ScriptMessageTarget &in target, const string &in topic, dictionary@ payload = null)
```

## Request and Reply

`Request` returns `AsyncTask<dictionary@>@`. The responder calls `Reply` or `Reject`.

```angelscript
ScriptMessageTarget clock = Message::Runtime("service.clock");
AsyncTask<dictionary@>@ task = Message::Request(ctx, clock, "clock.now", null, 300);
dictionary@ response;
Await(task, @response);
```

Responder:

```angelscript
void OnMessage(const ScriptMessage &in msg, const ScriptContext &in ctx) {
    if (msg.Topic() == "clock.now") {
        dictionary payload;
        payload["frame"] = "" + ctx.FrameIndex();
        Message::Reply(ctx, msg, payload);
    }
}
```

Failure response:

```angelscript
Message::Reject(ctx, msg, "not available");
```

## Payloads

Payloads are AngelScript dictionaries. Keep payload values simple: strings, numbers, booleans, object ids, and small arrays. For CK objects, prefer passing object ids or names and reacquiring safe refs with `Scene::*` in the receiver.

## Delivery Timing

Messages are processed by the manager during runtime ticks. Do not assume a publish call immediately invokes every receiver before the next line of code. Use `Request` and `Await` when the caller needs a response.

## Failure Modes

- `Publish`/`Send` return `false` when the manager cannot accept the message.
- Directed sends fail when the target cannot be resolved.
- Component sends fail when the captured target generation is stale.
- `Message::Component(ctx, id)` raises a script exception when no Component state currently owns that id.
- Requests can fail or time out; inspect the task state and `Error()`.
- A receiver can reject a request explicitly with `Message::Reject`.
