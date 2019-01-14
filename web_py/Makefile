
help:
	@echo 'Command:'
	@echo 'install    install the package.'
	@echo 'pack       pack the package only in local.'
	@echo 'upload     upload package to official pypi site.'
	@echo 'test       upload packge to test pypi site.'
	@echo 'clean      clean package files.'

install:
	python setup.py install

pack:
	python setup.py sdist --formats=gztar

# 打包上传到正式站点
upload:
	python setup.py sdist --formats=gztar upload

# 打包、注册、上传
test:
	#python setup.py sdist --formats=gztar -r pypitest upload
	twine upload --repository-url https://test.pypi.org/legacy/ dist/*

clean:
	rm -rf dist *.egg-info build __pycache__
