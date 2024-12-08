if __name__ == "__main__":
    link = """
    <a class="download" href="https://github.com/jesperkha/rum/releases/download/v{version}/RumInstaller.zip">
        <p>Download v{version}</p>
    </a>
    """

    version = input("Enter release version as x.x.x: ")
    with open("scripts/templates/index.html", "r") as f:
        txt = f.read()
        link = link.format(version=version)
        txt = txt.replace("{DOWNLOAD}", link)
        with open("index.html", "w+") as ff:
            ff.write(txt)