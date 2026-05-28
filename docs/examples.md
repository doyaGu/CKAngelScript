# Examples

These examples are small and copyable. They use current public signatures and avoid deprecated parameterless lifecycle callbacks.

## Runtime Script: Publish a Ready Message

```angelscript
[script id="example.ready" name="Ready Publisher" version="1.0.0" entry="runtime.as"]
[script.messages topics="example.ping"]
```

```angelscript
void OnLoad(const ScriptContext &in ctx) {
    print("Loaded " + ctx.Name());
}

void Update(const ScriptContext &in ctx) {
    if (ctx.FrameIndex() != 1) {
        return;
    }

    dictionary payload;
    payload["id"] = ctx.Id();
    Message::Publish(ctx, "example.ready", payload);
}
```

## Component: Inject Editor Values

```angelscript
class DoorLabel {
    [param type="string" default="Closed"]
    string Text;

    [param type="3dentity" name="Target"]
    CK3dEntity@ Target;

    void Start(const CKBehaviorContext &in ctx) {
        if (Target !is null) {
            print("Target is " + Target.GetName());
        }
    }
}
```

## Scene: Find, Create, Add, Destroy

```angelscript
void Update(const ScriptContext &in ctx) {
    Entity3DRef@ marker = Scene::FindOneEntity3D(ctx, "DebugMarker", true);
    if (marker is null || !marker.valid) {
        @marker = Scene::CreateEntity3D(ctx, "DebugMarker");
        if (marker !is null && marker.valid) {
            Scene::AddToCurrentScene(ctx, marker);
        }
    }

    if (marker !is null && marker.valid && ctx.FrameIndex() > 300) {
        Scene::Destroy(ctx, marker);
    }
}
```

## Messaging: Request and Reply

Responder:

```angelscript
[script id="service.clock" entry="runtime.as"]
[script.messages topics="clock.now"]
```

```angelscript
void OnMessage(const ScriptMessage &in msg, const ScriptContext &in ctx) {
    if (msg.Topic() != "clock.now") {
        return;
    }

    dictionary response;
    response["frame"] = "" + ctx.FrameIndex();
    Message::Reply(ctx, msg, response);
}
```

Caller:

```angelscript
void Start(const ScriptContext &in ctx) {
    dictionary request;
    request["source"] = ctx.Id();

    AsyncTask<dictionary@>@ task = Message::Request(ctx, "runtime:service.clock", "clock.now", request, 120);
    dictionary@ response;
    Await(task, @response);
    if (response !is null) {
        print("clock response received");
    }
}
```

## Async: Delay and Spawn

```angelscript
int ComputeScore() {
    return 42;
}

void Start(const ScriptContext &in ctx) {
    AsyncTask<void>@ wait = Async::Delay(30);
    Await(wait);

    AsyncTask<int>@ scoreTask = Async::Spawn(AsyncIntFunc(ComputeScore));
    int score = 0;
    Await(scoreTask, score);
    print("score=" + score);
}
```

## Behavior Bridge: Runtime BB Config

```angelscript
class TextOverlay {
    [bbconfig prototype="Interface/Text/2D Text" lifetime="component"]
    [bbsetting "Text Properties"="Screen Proportionnal,WordWrap"]
    [bbpin "Text"="Ready"]
    BBConfig@ Text;

    [bbslot from="Text" pin="Text"]
    BBSlot@ TextPin;

    BBInstance@ Instance;

    void Start(const CKBehaviorContext &in ctx) {
        @Instance = Text.EnsureStarted(ctx);
    }

    void Update(const CKBehaviorContext &in ctx) {
        if (Instance !is null) {
            Instance.StepSet(ctx, TextPin, "Frame running");
        }
    }
}
```

## Catalog Usage

After exporter validation:

```powershell
python tools\generate_angelscript_catalog.py --validation-dir build\validation\ballance
```

Script:

```angelscript
BBDecl@ componentDecl = CKASCatalog::BBHints::AngelScript_AngelScript_Component::Decl(ctx);
CKGUID stringType = CKASCatalog::ParamTypes::String();
```

The exact generated names depend on the exported host metadata. See [catalog-ballance.md](catalog-ballance.md).
