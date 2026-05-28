# Messaging

The `Message` namespace connects runtime scripts and AngelScript Components through the shared `ScriptMessage` type.

## Targets and Topics

Use topics for event names, and targets for directed messages.

| Target form | Meaning |
| --- | --- |
| `""` | Broadcast/publish according to topic subscriptions. |
| `runtime:<script-id>` | A runtime script instance. |
| `component:<CK_ID>` | A component behavior instance. |

Topic names should be stable strings such as `game.ready`, `ui.changed`, or `clock.now`.

## Registering Interest

Runtime scripts can declare topics in metadata:

```angelscript
[script id="hud.runtime" entry="runtime.as"]
[script.messages topics="game.ready;ui.changed"]
```

Runtime scripts and components can also subscribe dynamically:

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
Message::Send(ctx, "runtime:hud.runtime", "ui.changed", payload);
```

Both `ScriptContext` and `CKBehaviorContext` overloads are registered:

```angelscript
bool Publish(const ScriptContext &in ctx, const string &in topic, dictionary@ payload = null, const string &in target = "")
bool Publish(const CKBehaviorContext &in ctx, const string &in topic, dictionary@ payload = null, const string &in target = "")
bool Send(const ScriptContext &in ctx, const string &in target, const string &in topic, dictionary@ payload = null)
bool Send(const CKBehaviorContext &in ctx, const string &in target, const string &in topic, dictionary@ payload = null)
```

## Request and Reply

`Request` returns `AsyncTask<dictionary@>@`. The responder calls `Reply` or `Reject`.

```angelscript
AsyncTask<dictionary@>@ task = Message::Request(ctx, "runtime:service.clock", "clock.now", null, 300);
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
- Requests can fail or time out; inspect the task state and `Error()`.
- A receiver can reject a request explicitly with `Message::Reject`.
