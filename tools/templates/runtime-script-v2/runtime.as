class ExampleRuntime {
  void OnLoad(const ScriptRuntimeContext &in ctx) {
    ctx.Raise("loaded " + ctx.ScriptId());
  }

  void Awake(const ScriptRuntimeContext &in ctx) {}
  void OnEnable(const ScriptRuntimeContext &in ctx) {}
  void Start(const ScriptRuntimeContext &in ctx) {}
  void Update(const ScriptRuntimeContext &in ctx) {}
  void OnPostLoad(const ScriptRuntimeContext &in ctx) {}
  void OnPostProcess(const ScriptRuntimeContext &in ctx) {}
  void OnDisable(const ScriptRuntimeContext &in ctx) {}
  void OnDestroy(const ScriptRuntimeContext &in ctx) {}
  void OnReset(const ScriptRuntimeContext &in ctx) {}
  void OnPause(const ScriptRuntimeContext &in ctx) {}
  void OnResume(const ScriptRuntimeContext &in ctx) {}

  void OnMessage(const ScriptMessage &in msg, const ScriptRuntimeContext &in ctx) {
    if (msg.RequiresReply()) {
      Message::Reply(ctx, msg, msg.Payload());
    }
  }
}
