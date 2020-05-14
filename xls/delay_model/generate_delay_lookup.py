# Lint as: python3
#
# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Extracts delay model data from pbtxt, constructs lookup code used in C++."""

from absl import app
from absl import flags

import jinja2

from google.protobuf import text_format
from xls.common import runfiles
from xls.delay_model import delay_model
from xls.delay_model import delay_model_pb2

flags.DEFINE_string(
    'model_name', None,
    'Name of model. Should be a short string (e.g., "e"). Used as the '
    'identifier when accessing the mode via xls::GetDelayEstimator.')
flags.mark_flag_as_required('model_name')
FLAGS = flags.FLAGS


def main(argv):
  if len(argv) > 2:
    raise app.UsageError('Too many command-line arguments.')

  with open(argv[1], 'rb') as f:
    contents = f.read()

  dm = delay_model.DelayModel(
      text_format.Parse(contents, delay_model_pb2.DelayModel()))

  env = jinja2.Environment(undefined=jinja2.StrictUndefined)
  tmpl_text = runfiles.get_contents_as_text(
      'xls/delay_model/generate_delay_lookup.tmpl')
  template = env.from_string(tmpl_text)
  rendered = template.render(
      delay_model=dm,
      name=FLAGS.model_name,
      camel_case_name=''.join(
          s.capitalize() for s in FLAGS.model_name.split('_')))
  print('// DO NOT EDIT: this file is AUTOMATICALLY GENERATED and should not '
        'be changed.')
  print(rendered)


if __name__ == '__main__':
  app.run(main)
