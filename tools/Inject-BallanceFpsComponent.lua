local exec = require("nmo._executor")

local function env(name, default)
    local value = os.getenv(name)
    if value == nil or value == "" then
        return default
    end
    return value
end

local function env_int(name, default)
    local value = tonumber(env(name, ""))
    if value == nil then
        return default
    end
    return math.floor(value)
end

local component_guid = env("CKAS_COMPONENT_GUID", "5F5D4A84-3DFD4D19")
local parent_behavior_id = env_int("CKAS_FPS_PARENT_BEHAVIOR_ID", 0)
local parent_input_index = env_int("CKAS_FPS_MOUNT_INPUT_INDEX", 1)
local stress_components = env_int("CKAS_FPS_STRESS_COMPONENTS", 1)

if parent_behavior_id <= 0 then
    parent_behavior_id = assert(exec.root_script_id())
end
if parent_input_index <= 0 then
    error("CKAS_FPS_MOUNT_INPUT_INDEX must be 1-based")
end
if stress_components <= 0 then
    error("CKAS_FPS_STRESS_COMPONENTS must be positive")
end

local source = [[
class BallanceFpsSample {
    [bbconfig prototype="Interface/Text/2D Text" lifetime="component"]
    [bbsetting "Text Properties"="Screen Proportionnal,WordWrap"]
    [bbpin "Text"="CKAS FPS: ..."]
    BBConfig@ text;

    [bbslot from="text" pin="Text"]
    BBSlot@ textPin;

    Entity2DRef@ overlay;
    BBInstance@ instance;
    int frames = 0;
    float elapsedMs = 0.0f;

    void Start(const CKBehaviorContext &in ctx) {
        @overlay = Scene::CreateEntity2D(ctx, "CKAS FPS Overlay", true);
        if (overlay !is null && overlay.IsValid()) {
            text.Target(overlay.Entity2D());
        }

        @instance = text.EnsureStarted(ctx);
        if (ctx.Context !is null) {
            ctx.Context.OutputToConsole("[CKAS FPS] sample started", false);
        }
    }

    void Update(const CKBehaviorContext &in ctx) {
        frames++;
        elapsedMs += ctx.DeltaTime;
        if (elapsedMs < 1000.0f) {
            return;
        }

        float fps = 0.0f;
        if (elapsedMs > 0.0f) {
            fps = float(frames) * 1000.0f / elapsedMs;
        }
        string textValue = format("CKAS FPS: {:.1f}", fps);
        if (instance !is null && textPin !is null) {
            instance.StepSet(ctx, textPin, textValue);
        }
        if (ctx.Context !is null) {
            ctx.Context.OutputToConsole(textValue, false);
        }

        frames = 0;
        elapsedMs = 0.0f;
    }
}
]]

local start_io = assert(exec.io_at(parent_behavior_id, "input", parent_input_index))
for i = 1, stress_components do
    local suffix = ""
    if stress_components > 1 then
        suffix = " " .. tostring(i)
    end

    local op = assert(exec.add_node(
        parent_behavior_id,
        component_guid,
        "CKAS FPS Sample" .. suffix))
    assert(exec.set_parameter_value_from_handle(op, "input_param:Class", "BallanceFpsSample"))
    assert(exec.set_parameter_value_from_handle(op, "input_param:Source", source))
    assert(exec.add_behavior_link(
        parent_behavior_id,
        start_io,
        { operation = op, handle = "input:Enable" }))
end
