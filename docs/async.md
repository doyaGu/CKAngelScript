# Async Tasks

CKAngelScript async tasks provide cooperative suspension inside the script scheduler. They are frame-based, not OS threads.

## Core Type

`AsyncTask<T>` is a reference type. Common methods:

```angelscript
bool IsPending() const
bool IsRunning() const
bool IsCompleted() const
bool IsFailed() const
bool IsCancelled() const
bool IsDone() const
string Error() const
bool Cancel()
```

Use `Await` to suspend until a task finishes:

```angelscript
AsyncTask<void>@ wait = Async::Delay(30);
Await(wait);
```

Typed results use out parameters:

```angelscript
AsyncTask<int>@ task = Async::Spawn(AsyncIntFunc(GetScore));
int score = 0;
Await(task, score);
```

## Creating Tasks

Registered helpers:

```angelscript
AsyncTask<void>@ Async::Delay(int frames)
AsyncTask<void>@ Async::Spawn(AsyncVoidFunc@ fn)
AsyncTask<int>@ Async::Spawn(AsyncIntFunc@ fn)
AsyncTask<float>@ Async::Spawn(AsyncFloatFunc@ fn)
AsyncTask<string>@ Async::Spawn(AsyncStringFunc@ fn)
AsyncTask<CKObject@>@ Async::Spawn(AsyncObjectFunc@ fn)
```

`Async::Create` has the same overloads as `Spawn`. Generic out-parameter overloads exist for advanced cases:

```angelscript
void Async::Spawn(?&in fn, ?&out task)
void Async::Create(?&in fn, ?&out task)
```

## Aggregates

Supported aggregate helpers:

```angelscript
AsyncTask<void>@ Async::All(array<AsyncTask<void>@>@ tasks)
void Async::All(?&in tasks, ?&out task)
AsyncTask<void>@ Async::Race(array<AsyncTask<void>@>@ tasks)
void Async::Race(?&in tasks, ?&out task)
AsyncTask<void>@ Async::Any(array<AsyncTask<void>@>@ tasks)
void Async::Any(?&in tasks, ?&out task)
```

For typed arrays, use the generic out-parameter form:

```angelscript
array<AsyncTask<int>@> tasks;
AsyncTask<array<int>@>@ allInts;
Async::All(tasks, @allInts);

AsyncTask<int>@ firstInt;
Async::Race(tasks, @firstInt);
Async::Any(tasks, @firstInt);
```

The direct typed-return aggregate overloads are intentionally not exposed because they can make module compilation hang inside the Virtools Player host.

## Waiting on Bridge Tasks

Behavior Bridge tasks can be wrapped:

```angelscript
AsyncTask<void>@ Async::Wait(BBTask@ task)
AsyncTask<void>@ Async::Wait(GraphTask@ task)
AsyncTask<void>@ Async::Wait(const CKBehaviorContext &in ctx, BBTask@ task, int inputIndex = -1)
AsyncTask<void>@ Async::Wait(const CKBehaviorContext &in ctx, GraphTask@ task)
```

When code already runs with `BehaviorFrame@ frame`, prefer the frame-aware
wrappers so the script does not reconstruct `CKBehaviorContext` manually:

```angelscript
AsyncTask<void>@ frame.Wait(BBTask@ task, int inputIndex = -1)
AsyncTask<void>@ frame.Wait(GraphTask@ task)
AsyncTask<void>@ frame.WaitMessage(const string &in message, CKBeObject@ object = null)
AsyncTask<void>@ frame.WaitMessage(CKMessageType messageType, CKBeObject@ object = null)
AsyncTask<void>@ frame.LinkDelay(int frames)
```

Use `frame.LinkDelay(frames)` for a translated `CKBehaviorLink`
activation-delay wait. Use `Async::Delay(frames)` for ordinary script-authored
timing that is not standing in for a graph link.

## Cancellation

`Cancel()` asks the task to stop. Code waiting on the task should check failure/cancel state and read `Error()` when needed.

```angelscript
if (!task.Cancel()) {
    print("could not cancel: " + task.Error());
}
```

An `Await` registered with the scheduler has exactly one terminal result.
Lifecycle cancellation is reported as cancellation/failure to the suspended
invocation; removal of scheduler ownership is never interpreted as successful
completion. Polling code can distinguish `IsCancelled()` from `IsFailed()` and
read the stored reason through `Error()`.

## Scheduler Timing

Async tasks advance during manager processing. CK Pause freezes scheduler frame
advancement; Play continues from the remaining frame count. Reset, End, and CK
object clear cancel work that belongs to the preceding lifecycle epoch. A task
created by a Runtime Entry `OnReset` callback belongs to the new epoch and is
not removed by a late global scheduler clear. A task that waits for frames or
BB/graph progress will not complete until the host continues ticking. Avoid
blocking native code while waiting for a script task.

Each physical Runtime Entry generation and each Component or Executor instance
has a stable internal Script Host Scope owner. Lifecycle, message, and Behavior
Frame invocations propagate that owner to every task they create, including
nested `Async::Spawn` work. Reset cancels only the previous work for that owner
before calling `OnReset`; work owned by another host is unaffected, and tasks
created by the new `OnReset` invocation belong to the new Reset Epoch. Retiring
a Runtime Entry generation likewise cancels its owned tasks before unloading
its physical module without cancelling work owned by another Runtime Entry.
A repeated host Reset cancels tasks from the suspended `OnReset`, aborts that
prepared invocation, and opens a replacement Reset Epoch; only tasks created by
the replacement invocation remain owned by the current epoch.
