var vueapp = null;
function logfunc( Vue ) {
    vueapp = new Vue({
        data: {
            maindata: [],
            logType: {
                CONFCHANGE: '修改配置',
                CLIENT_LOGIN: '应用上线',
                CLIENT_CLOSE: '应用下线',
                PRVDREG: '服务注册',
                NOTIFY: '命令通知'
            }
        },

        methods: {
        },

        computed: {
            
        },

        created: function() {
            const url = "/syslog?size=300";
            let self = this;

            this.$http.get(url).then(function(resp){
                if (resp.body.code != 0) {
                    alert('抓取系统日志失败：'+url);
                    return;
                }
                for ( let logi in resp.body.data){
                    const timebeg_len = 19;
                    let logstr = resp.body.data[logi];
                    let seq1 = logstr.indexOf('|');
                    let logName = logstr.substr(timebeg_len+2, seq1-timebeg_len-2);

                    //console.log(logName);
                    self.maindata.push({
                        time: logstr.substr(0, 19),
                        logName: self.logType[logName],
                        contant: logstr.substr(seq1+1)
                    })
                }
            })
        },

        filters : {
        }
    }).$mount('#app')
};

logfunc(Vue);