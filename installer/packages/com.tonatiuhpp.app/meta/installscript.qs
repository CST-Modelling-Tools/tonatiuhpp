function tonatiuhExecutablePath()
{
    var path = installer.value("TargetDir") + "/bin/tonatiuhpp.exe";
    return path.replace(/\//g, "\\");
}

function quote(path)
{
    return "\"" + path + "\"";
}

function registerWindowsFileTypes()
{
    var executable = tonatiuhExecutablePath();
    var command = quote(executable) + " \"%1\"";
    var icon = executable + ",0";

    component.addOperation("RegisterFileType",
                           "tnhpp",
                           command,
                           "Tonatiuh++ project file",
                           "application/x-tonatiuhpp-project",
                           icon,
                           "ProgId=TonatiuhPP.Project");

    component.addOperation("RegisterFileType",
                           "tnhpps",
                           command,
                           "Tonatiuh++ script file",
                           "application/x-tonatiuhpp-script",
                           icon,
                           "ProgId=TonatiuhPP.Script");
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (installer.value("os") == "win")
        registerWindowsFileTypes();
}
