var vueapp = null;
function prvdfunc( Vue ) {
    vueapp = new Vue({
        data: {
            maindata: {},
            listContents: [],
            curPrvdName: '',
            readonly: false,
            addSample: { // 添加服务时的字段
                "show": false,
                "regname": "",
                "svrprop": {
                    "url": "",
                    "desc": "",
                    "protocol": 3,
                    "version": 0,
                    "weight": 100,
                    "idc": 0,
                    "rack": 0,
                    "enable": 1
                },
                "result": '',
                "resultStyle": {}
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
                this.addSample.result = "正在添加";

                const url = "/regsvr";
                let self = this;
                let postObj = self.addSample;
                this.$http.post(url, postObj).then(function(res){
                    if (res.body.code === 0) {
                        self.addSample.result = "成功 " + res.body.desc;
                        self.addSample.resultStyle.color = "green";
                        console.log(res.body.desc);
                    }else {
                        //alert('添加执行失败：' + JSON.stringify(res.body));
                        self.addSample.result = JSON.stringify(res.body);
                        self.addSample.resultStyle.color = "red";
                    }
                    self.submitting = false;
                }, function(){
                    //alert('添加请求' + url + '失败');
                    self.addSample.result = '添加请求' + url + '失败';
                    self.submitting = false;
                    self.addSample.resultStyle.color = "red";
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
                this.addSample.show = !this.addSample.show;
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