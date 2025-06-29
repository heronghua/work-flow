# my own configuration
alias vi='vim'
export REPO_URL='https://mirrors.tuna.tsinghua.edu.cn/git/git-repo'

#JAVA environment
#This /usr/bin/jdk is an link can be managed by update-alternative
export JAVA_HOME=/usr/bin/jdk
export PATH=${JAVA_HOME}/bin:${JAVA_HOME}/jre/bin:${PATH}
export CLASSPATH=.:${JAVA_HOME}/lib/dt.jar:${JAVA_HOME}/lib/tools.jar 

#python home environment
export PY_HOME=/usr/bin/py_home
export PATH=${PY_HOME}/bin:${PATH}

# for eclipse ide
alias eclipse='eclipse&'
export PATH=$PATH:/opt/eclipse

alias trace_processor='/opt/bin/trace_processor'

# for Android Sdk
export ANDROID_SDK_ROOT='/opt/android-sdk/'
export PATH=${ANDROID_SDK_ROOT}/tools:${ANDROID_SDK_ROOT}/platform-tools:${ANDROID_SDK_ROOT}/build-tools/30.0.3:$PATH

[ -f ~/.fzf.zsh ] && source ~/.fzf.zsh
#export TMPDIR=/home/heronghua/tmp

export PATH=/opt/git/bin:$PATH

# setting for aosp build
#export LC_ALL=C
export WITH_DEXPREOPT=false
#export TMPDIR=/home/heronghua/tmp
export BRANCH_TO_BUILD='android-8.1.0_r18'

#ollama
export OLLAMA_HOST=127.0.0.1:11434
