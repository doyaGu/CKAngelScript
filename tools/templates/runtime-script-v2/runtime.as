class ExampleRuntime {
  void OnLoad(const ScriptContext &in ctx) {
    ctx.Raise("loaded " + ctx.Id());
  }

  void Awake(const ScriptContext &in ctx) {}
  void OnEnable(const ScriptContext &in ctx) {}
  void Start(const ScriptContext &in ctx) {}
  void Update(const ScriptContext &in ctx) {}
  void OnPostLoad(const ScriptContext &in ctx) {}
  void OnPostProcess(const ScriptContext &in ctx) {}
  void OnDisable(const ScriptContext &in ctx) {}
  void OnDestroy(const ScriptContext &in ctx) {}
  void OnReset(const ScriptContext &in ctx) {}
  void OnPause(const ScriptContext &in ctx) {}
  void OnResume(const ScriptContext &in ctx) {}

  void OnMessage(const ScriptMessage &in msg, const ScriptContext &in ctx) {
    if (msg.RequiresReply()) {
      Message::Reply(ctx, msg, msg.Payload());
    }
  }
}
