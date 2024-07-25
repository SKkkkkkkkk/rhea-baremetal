import groovy.json.JsonOutput

def getGitBuildTag() {
    def tag = sh(
        script: "git describe --tags `git rev-list --tags --max-count=1` || true",
        returnStdout: true
    ).trim()
    if (tag.contains("fatal")) {
        return ""
    } else {
        return tag
    }
}

def getDirectory(String filePath) {
    int lastSeparatorIndex = filePath.lastIndexOf('/')
    return (lastSeparatorIndex != -1) ? filePath.substring(0, lastSeparatorIndex) : ""
}

pipeline {
    agent any

    environment {
        /* Build config */
        ARM_GNU_TOOLCHAIN_PATH = "/sw/tools/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf/bin"
        CMAKE_PATH = "/sw/tools/cmake-3.30.0-rc3-linux-x86_64/bin"
        /* Build info */
        GIT_REPO_NAME = sh(
            script: "basename \$(git remote get-url origin) .git",
            returnStdout: true
        ).trim()
        GIT_COMMIT_ID = sh(
            script: "git rev-parse --short HEAD",
            returnStdout: true
        ).trim()
        GIT_BUILD_TAG = getGitBuildTag()
        GIT_REPO_OWNER = sh(
            script: "git remote get-url origin | awk -F '/' '{print \$4}'",
            returnStdout: true
        ).trim()
        JFROG_REPO_PATH =   "_0002-a510-g124/sw/repo/" +
                            "${GIT_REPO_OWNER}/" +
                            "${GIT_REPO_NAME}/" +
                            "${BRANCH_NAME}/" +
                            "${GIT_COMMIT_ID}" +
                            "${ GIT_BUILD_TAG ? "_${GIT_BUILD_TAG}/" : "/" }"
        JFROG_FILE_PROPS =  "repo=${GIT_REPO_NAME};" +
                            "branch=${BRANCH_NAME};" +
                            "commit=${GIT_COMMIT_ID};" +
                            "tag=${GIT_BUILD_TAG};" +
                            "status=ok"
        BUILD_SCRIPTS = "apps/**/build.sh"
        UPLOAD_TARGET = "apps/**/build/*.bin"
    }

    stages {
        stage("build project") {
            steps {
                echo "build project"
                script {
                    def scripts = findFiles(glob: BUILD_SCRIPTS)
                    for (script in scripts) {
                        def scriptDir = getDirectory(script.path)
                        dir(scriptDir) {
                            sh(script:
                                "export PATH=\"${ARM_GNU_TOOLCHAIN_PATH}:${CMAKE_PATH}:\$PATH\"" +
                                " && ./build.sh"
                            )
                        }
                    }
                }
            }
        }
        stage("upload binary") {
            steps {
                echo "upload binaries"
                script {
                    def fileList = []
                    def files = findFiles(glob: UPLOAD_TARGET)
                    for (file in files) {
                        def path = sh(
                            script: "realpath ${file.path}",
                            returnStdout: true
                        ).trim()

                        fileList.add([
                            pattern: path,
                            target: JFROG_REPO_PATH,
                            props: JFROG_FILE_PROPS,
                            flat: "true"
                        ])
                    }

                    /* Assemble into JSON format */
                    def json = JsonOutput.toJson([files: fileList])
                    println(JsonOutput.prettyPrint(json))
                    /* Upload to artifactory*/
                    def rtServer = Artifactory.server "JFrog"
                    rtServer.upload(spec: json)

                    def buildInfo = Artifactory.newBuildInfo()
                    buildInfo.env.capture = true
                    buildInfo.env.collect()
                    rtServer.publishBuildInfo(buildInfo)
                }
            }
        }
    }
    post {
        always {
            echo "clean workspace"
            cleanWs(deleteDirs: true)
        }
    }
}
