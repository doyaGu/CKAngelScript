# CKAngelScript

CKAngelScript integrates the AngelScript scripting language into Virtools, providing developers with a flexible and powerful scripting environment for creating advanced and interactive 3D applications.

---

## Features

- **Full Bindings of the Virtools SDK**: Access and utilize the entire Virtools SDK through AngelScript for seamless integration of custom scripting with Virtools' extensive API.
- **AngelScript Integration**: Leverage the AngelScript scripting language to achieve greater flexibility and control in your 3D projects.
- **Building Blocks**:
  - **AngelScript Loader**: Load and unload AngelScript modules dynamically.
  - **AngelScript Runner**: Execute specific functions within AngelScript modules with precision.
- **Extensibility**: Expand Virtools' capabilities by integrating custom AngelScript modules.
- **Compatibility**: Supports Virtools 2.1 and higher.

---

## Building Blocks

### AngelScript Loader

The **AngelScript Loader** manages AngelScript modules, providing functionality to load and unload them dynamically.

#### Inputs
- **Load**: Activates the script loading process.
- **Unload**: Activates the script unloading process.

#### Outputs
- **Loaded**: Signals successful loading of a module.
- **Unloaded**: Signals successful unloading of a module.
- **Failed**: Signals a failure during the loading or unloading process.

#### Parameters
- **Name**: The name of the AngelScript module to load or unload.
- **Filename**: The file path of the AngelScript file.

#### Settings
- **Use File List**: Enable loading from a list of files.
- **Filename As Code**: Treat the filename parameter as code instead of a file path.
- **No Script Cache**: Disable caching for the loaded scripts (useful for debugging and development).

---

### AngelScript Runner

The **AngelScript Runner** executes specific functions within loaded AngelScript modules.

#### Inputs
- **In**: Activates the script function execution process.

#### Outputs
- **Out**: Signals successful execution of a function.
- **Error**: Signals that an error occurred during execution.

#### Parameters
- **Script**: The name of the loaded AngelScript module.
- **Function**: The name of the function to execute.

#### Settings
- **Output Error Message**: Outputs detailed error messages and stack traces when enabled.

---

## Getting Started

### Prerequisites

Before you begin, ensure the following:

1. **Virtools 2.1 or higher** is installed on your system.
2. **Microsoft Visual Studio** (any version compatible with Virtools SDK).
3. The **AngelScript SDK**, which must be prepared as described below.

---

### Preparing the AngelScript SDK

1. Download the AngelScript SDK from its [official website](http://www.angelcode.com/angelscript/).
2. Extract the downloaded SDK package.
3. Place the extracted contents into the `deps` directory under a folder named `angelscript`. Your directory structure should look like this:

---

### Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/doyaGu/CKAngelScript.git
   ```

2. **Initialize Submodules**:
   ```bash
   git submodule update --init --recursive
   ```

3. **Prepare the AngelScript SDK**:
   Complete the preparation steps outlined in the "Preparing the AngelScript SDK" section.

4. **Build the Project**:
   Open the project in **Microsoft Visual Studio**, configure the build settings to match your Virtools SDK installation, and build the project. The process will generate the following file:
   ```
   AngelScript.dll
   ```

5. **Integrate with Virtools**:
   Copy the `AngelScript.dll` file into the `BuildingBlocks` directory of your Virtools installation. For example:
   ```
   C:\Program Files\Virtools\BuildingBlocks\
   ```

6. Launch Virtools. The **AngelScript Loader** and **AngelScript Runner** building blocks will now be available in the Building Blocks palette.

---

## Usage

### Step 1: Write an AngelScript File

Create an AngelScript file (`example.as`) with your desired functionality. Below is an example demonstrating lifecycle events and interactions with the Virtools SDK:

#### Example Script: `example.as`

```cpp
void OnLoad() {
    print("OnLoad!");
}

void OnUnload() {
    print("OnUnload!");
}

void OnPause() {
    print("OnPause!");
}

void OnResume() {
    print("OnResume!");
}

void OnReset() {
    print("OnReset!");
}

int main(const CKBehaviorContext &in behContext) {
    print("Hello from AngelScript!");

    CKContext@ context = behContext.Context;

    print("3D entities:");
    XObjectPointerArray objects = context.GetObjectListByType(CKCID_3DENTITY, true);
    for (int i = 0; i < objects.Size(); i++) {
        CK3dEntity@ entity = cast<CK3dEntity>(objects[i]);
        print(entity.GetName());
    }
    print("3D entities end.");

    return CKBR_OK;
}
```

---

### Step 2: Load the Script in Virtools Dev

1. Open **Virtools Dev** and create a new Behavior or edit an existing one.
2. Drag the **AngelScript Loader** building block into your Behavior from the **Building Blocks** palette.
3. Configure the **Name** parameter with a unique identifier (e.g., `example_script`) and provide the **Filename** parameter with the path to your script file (`example.as`).
4. Connect a trigger to the **Load** input to activate the loading process.
5. Verify successful loading by checking for the `OnLoad!` message in the Virtools console.

---

### Step 3: Execute a Function

1. Drag the **AngelScript Runner** building block into your Behavior.
2. Set the **Script** parameter to the module name specified in the Loader (`example_script`).
3. Set the **Function** parameter to the function you wish to execute, e.g., `main`.
4. Connect a trigger to the **In** input to activate the function.
5. Observe the console for outputs from your script.

---

### Step 4: Unload the Script

1. Use the **Unload** input on the **AngelScript Loader** to deactivate the module.
2. Verify that the `OnUnload` function is called, and the module is unloaded.

---

### Debugging and Testing

- **Console Output**: Use `print` statements in your AngelScript code for logging and debugging.
- **Error Messages**: Enable the **Output Error Message** setting in the Runner block to get detailed stack traces and error messages.

---

## Example Workflow in Virtools Dev

1. **Create and Save a Script**: Write your AngelScript code in a `.as` file and save it to your project directory.
2. **Load the Script**: Use the **AngelScript Loader** to load the script file.
3. **Run Script Functions**: Use the **AngelScript Runner** to execute specific functions from the loaded module.
4. **Unload the Script**: Use the **Unload** input of the Loader to clean up resources.

By following these steps, you can fully utilize AngelScript within Virtools Dev to enhance your projects.

---

## Contributing

Contributions to CKAngelScript are welcome. To contribute:

1. Fork this repository on GitHub.
2. Make your changes.
3. Submit a pull request with a detailed explanation of your modifications.

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- **[AngelScript](http://www.angelcode.com/angelscript/)**: The scripting language integrated into this project.
- **[Virtools](https://en.wikipedia.org/wiki/Virtools/)**: The 3D engine platform extended by CKAngelScript.