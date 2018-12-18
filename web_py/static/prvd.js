var vueapp = null;
function prvdfunc( Vue ) {
    vueapp = new Vue({
        data: {
            maindata: {},
            listContents: [],
            curPrvdName: '',
            readonly: false,
            addSample: { // 添加服务时的字段
                "regname": "",
                "svrprop": {
                    "url": "",
                    "prvdid": 0,
                    "desc": "",
                    "protocol": 3,
                    "version": 0,
                    "weight": 100,
                    "idc": 0,
                    "rack": 0,
                    "enable": 1
                }
            },
            addSampleCtrl:{
                show: false,
                result: '',
                resultStyle: {},
                textMsg: {
                    url: ["url样例：\n1. tcp://192.168.1.68:2000\n2. udp://192.168.1.68:2000\n3. http://192.168.1.68:2000\n4. https://192.168.1.68:2000"],
                    "prvdid": ["默认0，如果需要注册多个同名服务则以此值区分"],
                    "desc": ["服务提供者描述信息"],
                    "protocol": ["protocol取值:  tcp=1 udp=2 http=3 https=4"],
                    "version": ["版本信息，当为0时服务发现查询不去匹配，若大于0则会严格匹配"],
                    "weight": ["服务权重值，越大则服务调用者应调用的概率越高"],
                    "idc": ["机房编号"],
                    "rack": ["机架编号"],
                    "enable": ["是否启用：启用1，禁用0"]
                }
            },

            searchData: {
                svrSelect: '服务列表',
                searchResult: ''
            }
        },

        methods: {
            isKeyReadOnly: function(key){
                return !(key in {weight:1, enable:1})
            },

            onClickName: function(pname){
                this.curPrvdName = pname;
                this.listContents = this.maindata[pname];
                this.readonly = false;
            },

            onItemValChange: function(event, itmobj, itmKey){
                console.log("change to " + itmobj);
                let targeVal = parseInt(event.target.value);
                if (!targeVal) {
                    alert("invalid value " + targeVal);
                    return;
                }

                let paramObj = {
                    svrid: itmobj.svrid,
                    regname: itmobj.regname,
                    prvdid: itmobj.prvdid
                };
                paramObj[itmKey] = targeVal;
                let url = "/notify/provider";
                let self = this;
                this.$http.get(url, {params:paramObj}).then(function(res){
                    if (res.body instanceof Object) {
                        //self.maindata = res.body.data;
                    }else {
                        alert('修改' + itmKey + '执行失败：' + JSON.stringify(res.body));
                    }
                }, function(){
                    alert('修改' + itmKey + '请求' + url + '失败');
                })
            },

            onSearchPrvd: function(){
                const url = "/qsvr?bookchange=0&regname=" + this.searchData.svrSelect;
                let self = this;
                self.searchData.searchResult = '正在查询: ' + this.searchData.svrSelect;
                this.$http.get(url).then(function(res){
                    if (res.body instanceof Object && res.body.code == 0) {
                        self.listContents = res.body.data;
                        self.readonly = true;
                        self.searchData.searchResult = "成功查到" + self.listContents.length + "项";
                        
                    }else {
                        self.searchData.searchResult = ('执行失败：' + JSON.stringify(res.body));
                    }
                }, function(){
                    console.error('请求' + url + '失败');
                    self.searchData.searchResult = '请求' + url + '失败';
                })
            },

            onAddPrvd: function() {
                if (this.submitting){
                    alert("处理中，请等待. . .")
                    return;
                }
                this.submitting = true;
                this.addSampleCtrl.result = "正在添加";

                const url = "/regsvr";
                let self = this;
                let postObj = self.addSample;
                this.$http.post(url, postObj).then(function(res){
                    if (res.body.code === 0) {
                        self.addSampleCtrl.result = "成功 " + res.body.desc;
                        self.addSampleCtrl.resultStyle.color = "green";
                        console.log(res.body.desc);
                    }else {
                        //alert('添加执行失败：' + JSON.stringify(res.body));
                        self.addSampleCtrl.result = JSON.stringify(res.body);
                        self.addSampleCtrl.resultStyle.color = "red";
                    }
                    self.submitting = false;
                }, function(){
                    //alert('添加请求' + url + '失败');
                    self.addSampleCtrl.result = '添加请求' + url + '失败';
                    self.submitting = false;
                    self.addSampleCtrl.resultStyle.color = "red";
                })
            },

            onRefresh: function(){
                const url = "/svrall?regname=all";
                let self = this;
                this.$http.get(url).then(function(res){
                    if (res.body instanceof Object) {
                        self.maindata = res.body.data;
                    }else {
                        console.log('执行失败：' + JSON.stringify(res.body));
                    }
                }, function(){
                    console.error('请求' + url + '失败');
                })
            },

            onShowAddClick: function(){
                this.addSampleCtrl.show = !this.addSampleCtrl.show;
            }
        },

        computed: {
            
        },

        created: function() {
            this.onRefresh();
        },

        filters : {
            toHumanTime : function(val) {
              var d = new Date(val * 1000); 
              var date = (d.getFullYear()) + "-" + 
                         (d.getMonth() + 1) + "-" +
                         (d.getDate()) + " " + 
                         (d.getHours()) + ":" + 
                         (d.getMinutes()) + ":" + 
                         (d.getSeconds());
              return date;
            }
        }
    }).$mount('#app')
};

prvdfunc(Vue);