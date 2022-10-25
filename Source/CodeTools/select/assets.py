# This file is part of CSS Engine
# Licensed under the MIT License,
# http://www.opensource.org/licenses/mit-license.php
# Copyright 2017 Lucas Neves <lcneves@gmail.com>
# Copyright (C) 2022 Beijing FMSoft Technologies Co., Ltd.

copyright = '''\
/*
 * This file is part of CSS Engine
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2017 The NetSurf Project
 * Copyright 2022 The HVML Project
 */
'''

assets = {}

assets['computed.h'] = {}
assets['computed.h']['header'] = copyright
assets['computed.h']['footer'] = ''

assets['propset.h'] = {}
assets['propset.h']['header'] = copyright + '''
/** Default values are 'initial value', unless the property is inherited,
 *  in which case it is 'inherit'. */'''
assets['propset.h']['footer'] = ''

assets['propget.h'] = {}
assets['propget.h']['header'] = copyright
assets['propget.h']['footer'] = ''

