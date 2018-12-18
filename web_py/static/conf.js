var vueapp = null;
function conffun( Vue ) {
    vueapp = new Vue({
        data: {
            tabidx: 0,
            filenames: {}, // {"f1": [isdel, mtime, contants, modifyfg, timehightlight]}
            selfile: '', // 选中的文件
            seltime: '',
            contents: '',
            addflag: 0,
            newfilename: '',
            qctrl: {
                qpanel: false,
                cond: '',
                valid: false,
                filename: '文件列表',
                incbase: 0,
                output: '',
                sdkcall: {"sdk_cpp": "", "http": "", "tcp": ""}
            }
        },

        methods: {
            getFileName: function() {
                const url = "/confname";
                let self = this;
                this.$http.get(url).then(function(res){
                    if (res.body instanceof Object) {
                        self.filenames = res.body;
                        console.log(self.filenames.toString());
                    }else {
                        console.log('执行失败：' + JSON.stringify(res.body));
                    }
                }, function(){
                    console.error('请求' + url + '失败');
                })
            },

            getContanct: function(fname) {
                const url = '/getconf?key_pattern=/&file_pattern=' + fname;
                let self = this;
                this.$http.get(url).then(function(resp){
                    if (resp.body.code === 0) {
                        const filename = resp.body.file_pattern;
                        self.filenames[filename][1] =  resp.body.mtime;
                        self.filenames[filename][2] =  JSON.stringify(resp.body.contents, null, 4);
                        self.filenames[filename][3] =  false; // 修改标记
                        self.filenames[filename][4] = '';

                        self.contents = JSON.stringify(resp.body.contents, null, 4);
                    } else {
                        console.error('执行失败: ' + JSON.stringify(res.body));
                    }
                }, function(){
                    console.error('请求' + url + '失败');
                })
            },

            chooseFile: function(fname){
                if (this.selfile && fname != this.selfile && this.filenames[this.selfile][3]) {
                    // 发生了修改时切换
                    // 暂存
                    this.filenames[this.selfile][2] = this.contents;
                }

                this.selfile = fname;
                this.seltime = this.filenames[fname][1];
                if (this.filenames[fname].length < 3) {
                    this.getContanct(fname);
                }

                this.contents = this.filenames[fname][2];
            },

            refresh: function(){
                if (!this.selfile) {
                    alert('no select file');
                    return;
                }
                if (this.filenames[this.selfile][3]){ // 修改过，提示
                    if (!confirm('文件:' + this.selfile + '修改过，确定继续会被覆盖')){
                        return;
                    }
                }

                this.getContanct(this.selfile);
            },

            modify: function(){ // 向后台提交修改
                if (!this.selfile) {
                    alert('no select file');
                    return;
                }

                if (!this.filenames[this.selfile][3]){
                    console.log(this.selfile + " no change return;");
                    return;
                }

                const url = '/setconf';
                // json合法性检查
                if (!this.jsonValid(this.contents)){
                    alert("无效的json格式");
                    return;
                }
                if (!confirm('提交修改'+this.selfile+"?")){
                    return;
                }

                let self = this;
                let opFilename = this.selfile;
                this.filenames[opFilename][2] = this.contents;

                this.$http.post(url, {
                    filename: opFilename,
                    contents: JSON.parse(this.contents)
                }).then(function(resp){
                    if (resp.body.code === 0){
                        console.log('修改' + opFilename + '成功, ' + self.filenames[opFilename][3]);
                        self.filenames[opFilename][1] = resp.body.mtime;
                        //self.$set(self.filenames[opFilename], 3, false);
                        self.filenames[opFilename][3] = false;
                        self.$set(self.filenames[opFilename], 4, 'time_hl');
                        this.seltime = resp.body.mtime;
                    } else {
                        alert('响应失败:' + resp.body.desc);
                    }
                }, function(){
                    alert('POST失败' + url);
                });
            },

            addClick: function(flag){ // 点击新增
                this.addflag = 1;
                let newfilename = this.newfilename;
                if (2 === flag){
                    if (newfilename.length < 3) {
                        alert('Invalid FileName');
                        return ;
                    }

                    var item = this.filenames[newfilename];
                    if (item){
                        alert('已存在的文件名：'+ newfilename);
                        return ;
                    }

                    //this.filenames[newfilename] = [1, 1, "", 0, 0];
                    const now = Date.parse(new Date()) / 1000;
                    this.$set(this.filenames, newfilename, [1, now, "", 0, 'newfile']);
                    this.chooseFile(newfilename);
                    this.newfilename = '';
                    this.addflag = 0;
                }
            },

            inputChange: function(istrue){
                if (this.selfile in this.filenames){
                    this.filenames[this.selfile][3] = istrue;
                }
            },
            
            jsonValid: function(str) {
                if (typeof str == 'string') {
                    try {
                        var obj=JSON.parse(str);
                        return (typeof obj == 'object' && obj );
                    } catch(e) {
                        console.error('error：'+str+'!!!'+e);
                        return false;
                    }
                }
                console.log('It is not a string!')
            },

            removeClick: function(filename) {
                console.log("click remove " + filename);
                if (!confirm("确认要删除文件:" + filename)) {
                    return ;
                }
                
                const url = '/setconf';
                let self = this;
                this.$http.post(url, { filename: filename }
                ).then(function(resp){
                    if (resp.body.code === 0){
                        self.$set(self.filenames[filename], 0, 0);
                        console.log("成功删除" + filename + " " + JSON.stringify(resp.body));
                    } else {
                        alert("删除" + filename + "失败 " + resp.body.desc)
                    }
                }, function(){
                    alert("请求" + filename + "删除失败")
                });
            },

            onQueryClick: function(){
                if (!this.qctrl.valid) {
                    alert('条件无效不能查询')
                    return false;
                }

                let curFileName = this.qctrl.filename == '文件列表'? '': this.qctrl.filename;
                let qctrlkey = this.qctrl.key? this.qctrl.key : '/';
                let url = '/getconf?file_pattern=' + curFileName;
                url += "&key_pattern=" + qctrlkey;
                url += "&incbase=" + (this.qctrl.incbase? "1": "0");
                let self = this;
                this.$http.get(url).then(function(resp){
                    if (resp.body.code == 0) {
                        self.qctrl.cond = url;
                        self.qctrl.output = JSON.stringify(resp.body.contents, null, 4);
                        console.log(resp.body.contents);
                    } else {
                        alert('请求' + url + '失败' + JSON.stringify(resp.body));
                    }
                }, function(){
                    alert('请求' + url + '失败');
                })

                return false;
            }
        },

        computed: {
            qcondiction: function() {
                let curFileName = this.qctrl.filename == '文件列表'? '': this.qctrl.filename;
                let qctrlkey = this.qctrl.key? this.qctrl.key : '/';
                let cond = curFileName + qctrlkey;
                cond += '&inbase=' + this.qctrl.incbase;

                let httpparam =  {
                    "file_pattern": curFileName,
                    "key_pattern":  qctrlkey,
                    "incbase": (this.qctrl.incbase? "1": "0")
                };

                this.qctrl.valid = (curFileName in this.filenames) && 
                    (qctrlkey && qctrlkey.length > 0 && 
                        qctrlkey.search(/^[-/\w]+$/) >= 0);

                this.qctrl.sdkcall["sdk_cpp"] =  'client_c::Query(oval, \"' + curFileName + qctrlkey + '\", true)';
                this.qctrl.sdkcall["http"] = '/getconf?file_pattern=' + curFileName
                 + "&key_pattern=" + qctrlkey + "&incbase=" + (this.qctrl.incbase? "1": "0");
                this.qctrl.sdkcall["tcp"] = "cmdid=CMD_GETCONFIG_REQ; body=" + JSON.stringify(httpparam);

                return cond;
            }
        },

        created: function() {
            this.getFileName();
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

conffun(Vue);